#include <iostream>
#include <fstream>
#include <unistd.h>
#include "dns.h"
#include <sys/socket.h>
#include <sys/types.h>
#include <bits/stdc++.h>
#include <netdb.h>
#include <arpa/inet.h>

using namespace std;
int main(int argc, char **argv)
{
    char *server;
    int port = 53;
    char *filter;
    int arg;
    char ipstr[INET6_ADDRSTRLEN];

    while ((arg = getopt(argc, argv, "s:p:f:")) != -1)
    {
        switch (arg)
        {
        case 's':
            server = optarg;
            break;
        case 'p':
            port = atoi(optarg);

            break;
        case 'f':
            filter = optarg;
            break;
        }
    }

    cout << "parameters specified server: " << server
         << " port: " << port
         << " filter_file: " << filter << endl;

    ifstream file;
    string line;
    if (filter != NULL)
    {
        file.open(filter);
        if (file.is_open())
        {
            while (getline(file, line))
            {
                // cout << line << '\n';
            }
            file.close();
        }
        else
        {
            cout << "Unable to open file" << endl;
        }
    }

    int status;
    struct addrinfo hints;
    struct addrinfo *servinfo;

    memset(&hints, 0, sizeof(hints));

    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE;

    if ((status = getaddrinfo(server, "3490", &hints, &servinfo)) != 0)
    {
        fprintf(stderr, "gai error: %s\n", gai_strerror(status));
        exit(1);
    }

    struct addrinfo *p;

    for (p = servinfo; p != NULL; p = p->ai_next)
    {
        void *addr;
        char *ipver;
        struct sockaddr_in *ipv4;
        struct sockaddr_in6 *ipv6;

        // get the pointer to the address itself,
        // different fields in IPv4 and IPv6:
        if (p->ai_family == AF_INET)
        { // IPv4
            ipv4 = (struct sockaddr_in *)p->ai_addr;
            addr = &(ipv4->sin_addr);
            ipver = "IPv4";
        }
        else
        { // IPv6
            ipv6 = (struct sockaddr_in6 *)p->ai_addr;
            addr = &(ipv6->sin6_addr);
            ipver = "IPv6";
        }

        // convert the IP to a string and print it:
        inet_ntop(p->ai_family, addr, ipstr, sizeof ipstr);
        printf("  %s: %s\n", ipver, ipstr);
    }

    int sockfd = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);
    bind(sockfd, servinfo->ai_addr, servinfo->ai_addrlen);

    listen(sockfd, 0);
    int clientSocket = accept(sockfd, nullptr, nullptr);
        char buffer[1024] = {0};
        recv(clientSocket, buffer, sizeof(buffer), 0);
        cout << "Message from client: " << buffer << endl;
    
    close(sockfd);
    freeaddrinfo(servinfo);

    // create a socket
    // bind it to my own ip with the port specified
    // create socket for the resolver
    // bind the resolver socker
    // communicate with the resolver
    // send reply back to the one making the rerquest
    return 0;
}