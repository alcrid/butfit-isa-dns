#include <bits/stdc++.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include "client.hpp"
#include <netdb.h>
#include <map>
#include <bits/stdc++.h>
#include "dns_message.hpp"

// Driver code
int main()
{
    int sockfd;
    struct addrinfo hints, *servinfo, *p;
    int rv, numbytes;
    std::vector<uint8_t> buffer;
    std::vector<std::string> test = {"test.com", "test2.com"};
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE;
    
    if ((rv = getaddrinfo(NULL, "3490", &hints, &servinfo)) != 0)
    {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    for (p = servinfo; p != NULL; p = p->ai_next)
    {
        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1)
        {
            perror("socket error");
            continue;
        }
        break;
    }

    if (p == NULL)
    {
        fprintf(stderr, "client failed to create a socket \n");
        return 1;
    }

    createDnsPacket(0x1234, buffer, test);

    size_t offset1 = 0;
    parseDnsMessage(buffer, offset1);

    if ((numbytes = sendto(sockfd, buffer.data(), buffer.size(), 0,
                           p->ai_addr, p->ai_addrlen)) == -1)
    {
        perror("talker: sendto");
        exit(1);
    }

    struct sockaddr_storage their_addr;
    std::vector<uint8_t> buff(100);
    char s[INET_ADDRSTRLEN];
    socklen_t addr_len;
    addr_len = sizeof their_addr;
    if ((numbytes = recvfrom(sockfd, buff.data(), buff.size(), 0,
                             (struct sockaddr *)&their_addr, &addr_len)) == -1)
    {
        perror("recvfrom");
        exit(1);
    }

    printf("listener: packet is %d bytes long\n", numbytes);
    printf("listener: packet contains : ");
    size_t offset = 0;
    
    parseDnsMessage(buff, offset);

    return 0;
}