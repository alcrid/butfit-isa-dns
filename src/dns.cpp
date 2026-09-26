#include "dns_filter.hpp"
#include "blocklist.hpp"
#include <iostream>
#include <cstdlib>
#include <unistd.h>
#include "logger.hpp"

void signal_handler(int)
{
    Logger::log("Shutdown signal received");
    keep_running = 0;
}

int main(int argc, char **argv)
{
    char *server = nullptr;
    char *port = (char *)"53";
    char *filter = nullptr;
    bool port_set = false;
    int arg;
    // Parse arguments
    while ((arg = getopt(argc, argv, "s:p:f:vh")) != -1)
    {
        switch (arg)
        {
        case 's':
            if (server != nullptr)
            {
                std::cerr << "Duplicate parameters not allowed" << std::endl;
                exit(1);
            }
            server = optarg;
            break;
        case 'p':
            if (port_set)
            {
                std::cerr << "Duplicate parameters not allowed" << std::endl;
                exit(1);
            }
            port_set = true;
            port = optarg;
            if (atoi(port) < 1 || atoi(port) > 65535)
            {
                exit(1);
            }
            break;
        case 'f':
            if (filter != nullptr)
            {
                // duplicate parameters not allowed
                std::cerr << "Duplicate parameters not allowed" << std::endl;
                exit(1);
            }
            filter = optarg;
            break;
        case 'v':
            Logger::set_verbose(true);
            break;
        case 'h':
            std::cout << "Usage: ./dns -s server [-p port] -f filter [-v] [-h]" << std::endl;
            exit(0);
            break;
        default:
            exit(1);
        }
    }

    if (server == nullptr)
    {
        Logger::log("Error: -s <server> parameter is required");
        exit(1);
    }

    if (filter == nullptr)
    {
        exit(1);
    }

    Logger::log("DNS Filter Starting");
    Logger::log("Server: " + std::string(server));
    Logger::log("Port: " + std::string(port));
    Logger::log("Filter file: " + std::string(filter));

    // Load blocklist
    Blocklist blocklist;
    if (filter != nullptr)
    {
        blocklist.load_from_file(filter);
    }

    struct sigaction sa;
    sa.sa_handler = signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, nullptr);
    sigaction(SIGTERM, &sa, nullptr);

    // run the filter
    run_dns_filter(server, port, blocklist);

    return 0;
}