#include "dns_filter.hpp"
#include "logger.hpp"

volatile sig_atomic_t keep_running = 1;

void *handle_query(void *threadarg)
{
    pthread_detach(pthread_self());
    thread_data *data = static_cast<thread_data *>(threadarg);

    // check if the program was shut down
    if (!keep_running)
    {
        delete data;
        return nullptr;
    }

    // unpack args
    std::vector<uint8_t> query_packet = data->packet;
    struct sockaddr_storage client_addr = data->client_addr;
    socklen_t addr_len = data->addr_len;
    int sockfd_local = data->sockfd_local;
    const char *server = data->server;
    const Blocklist *blocklist = data->blocklist;
    delete data;

    std::string domain_name;
    error_type error = parse_dns_message(query_packet, domain_name);

    if (error != NO_ERROR)
    {
        create_error_packet(query_packet, error);
        sendto(sockfd_local, query_packet.data(), query_packet.size(), 0,
               (struct sockaddr *)&client_addr, addr_len);
        Logger::log("Sent error response to client");
        return nullptr;
    }

    // check blocklist
    if (blocklist->is_blocked(domain_name))
    {
        Logger::log("Domain blocked!");
        create_error_packet(query_packet, REFUSED);
        sendto(sockfd_local, query_packet.data(), query_packet.size(), 0,
               (struct sockaddr *)&client_addr, addr_len);
        return nullptr;
    }

    struct addrinfo *resolver_info;
    int sockfd_resolver = create_resolver_socket(server, &resolver_info);

    if (sockfd_resolver == -1)
    {
        return nullptr;
    }

    int attempt = 0;
    std::vector<uint8_t> response(MAXUDPSIZE);
    // set socket timeout
    struct timeval tv;
    tv.tv_sec = TIMEOUT / 1000;
    tv.tv_usec = (TIMEOUT % 1000) * 1000;
    setsockopt(sockfd_resolver, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    // forward to resolver
    while (attempt <= MAXREPEATS)
    {

        if (sendto(sockfd_resolver, query_packet.data(), query_packet.size(), 0,
                   resolver_info->ai_addr, resolver_info->ai_addrlen) == -1)
        {
            Logger::log(std::string("sendto (resolver): ") + strerror(errno));
            break;
        }

        // get response
        int response_bytes = recvfrom(sockfd_resolver, response.data(),
                                      response.size(), 0, nullptr, nullptr);

        if (response_bytes >= 0)
        {
            sendto(sockfd_local, response.data(), response_bytes, 0,
                   (struct sockaddr *)&client_addr, addr_len);
            Logger::log("Sent response to client");
            close(sockfd_resolver);
            freeaddrinfo(resolver_info);
            return nullptr;
        }

        if (errno == EAGAIN || errno == EWOULDBLOCK)
        {
            // Timed out, retry
            attempt++;
            continue;
        }

        if (errno == EINTR)
        {
            continue;
        }
    }

    create_error_packet(query_packet, SERVER_FAILURE);
    sendto(sockfd_local, query_packet.data(), query_packet.size(), 0,
           (struct sockaddr *)&client_addr, addr_len);
    close(sockfd_resolver);
    freeaddrinfo(resolver_info);

    return nullptr;
}

int create_listening_socket(const char *port)
{
    int sockfd;
    struct addrinfo hints, *servinfo, *p;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE;

    int rv;
    if ((rv = getaddrinfo(NULL, port, &hints, &servinfo)) != 0)
    {
        Logger::log("getaddrinfo error: " + std::string(gai_strerror(rv)));
        exit(1);
    }

    for (p = servinfo; p != NULL; p = p->ai_next)
    {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                             p->ai_protocol)) == -1)
        {
            Logger::log("listener: socket: " + std::string(strerror(errno)));
            continue;
        }

        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1)
        {
            close(sockfd);
            Logger::log("listener: bind: " + std::string(strerror(errno)));
            continue;
        }
        break;
    }

    if (p == NULL)
    {
        Logger::log("Failed to bind socket");
        freeaddrinfo(servinfo);
        exit(1);
    }

    freeaddrinfo(servinfo);
    return sockfd;
}

int create_resolver_socket(const char *server,
                           struct addrinfo **resolver_info)
{
    int sockfd;
    struct addrinfo hints;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;

    int rv;
    if ((rv = getaddrinfo(server, "53", &hints, resolver_info)) != 0)
    {
        Logger::log("getaddrinfo (resolver) error: " +
                    std::string(gai_strerror(rv)));
        return -1;
    }

    if ((sockfd = socket((*resolver_info)->ai_family,
                         (*resolver_info)->ai_socktype,
                         (*resolver_info)->ai_protocol)) == -1)
    {
        Logger::log("socket (resolver): " + std::string(strerror(errno)));
        freeaddrinfo(*resolver_info);
        return -1;
    }

    return sockfd;
}

void run_dns_filter(const char *server, const char *port, const Blocklist &blocklist)
{
    struct addrinfo *test_resolver_info;
    int sockfd_local = create_listening_socket(port);

    // check if resolver address is correct
    int sockfd_resolver_test = create_resolver_socket(server, &test_resolver_info);
    if (sockfd_resolver_test == -1)
    {
        Logger::log("ERROR: Cannot connect to upstream resolver '" +
                    std::string(server) + "'. Exiting.");
        close(sockfd_local);
        exit(1);
    }

    close(sockfd_resolver_test);
    freeaddrinfo(test_resolver_info);
    std::vector<uint8_t> buf(MAXUDPSIZE);

    while (keep_running)
    {
        struct sockaddr_storage client_addr;
        socklen_t addr_len = sizeof(client_addr);

        int numbytes = recvfrom(sockfd_local, buf.data(), buf.size(), 0,
                                (struct sockaddr *)&client_addr, &addr_len);

        if (numbytes <= 0)
        {
            if (numbytes == -1 && errno == EBADF)
            {
                // Socket was shut down
                break;
            }
            if (numbytes == -1)
            {
                Logger::log("recvfrom: " + std::string(strerror(errno)));
            }
            continue;
        }
        thread_data *data = new thread_data{
            std::vector<uint8_t>(buf.begin(), buf.begin() + numbytes),
            client_addr,
            addr_len,
            sockfd_local,
            server,
            &blocklist};

        // create the thread to handle the query
        pthread_t thread;
        if (pthread_create(&thread, nullptr, handle_query, data) != 0)
        {
            Logger::log("pthread_create failed");
            delete data;
        }
    }

       Logger::log("Shutting down gracefully...");
    shutdown(sockfd_local, SHUT_RDWR);
    close(sockfd_local);
}
