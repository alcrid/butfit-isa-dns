#ifndef DNS_FILTER_H
#define DNS_FILTER_H

#include "blocklist.hpp"
#include <vector>
#include <vector>
#include <string>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include "dns_message.hpp"
#include <signal.h>

// timeout in ms
#define TIMEOUT 200
// max repeats of dns query
#define MAXREPEATS 3
#define MAXUDPSIZE 65535

extern volatile sig_atomic_t keep_running;

struct thread_data
{
    std::vector<uint8_t> packet;
    struct sockaddr_storage client_addr;
    socklen_t addr_len;
    int sockfd_local;
    const char *server;
    const Blocklist *blocklist;
};

int create_listening_socket(const char *port);
int create_resolver_socket(const char *server, struct addrinfo **resolver_info);
void *handle_query(void *threadarg);
void run_dns_filter(const char *server, const char *port, const Blocklist &blocklist);
#endif