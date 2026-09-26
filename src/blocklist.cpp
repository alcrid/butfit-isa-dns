#include "blocklist.hpp"
#include <fstream>
#include <iostream>
#include <algorithm>
#include "logger.hpp"

std::string Blocklist::normalize_domain(const std::string &domain) const
{
    std::string normalized = domain;

    size_t start = normalized.find_first_not_of(" \t\r\n");
    size_t end = normalized.find_last_not_of(" \t\r\n");

    if (start != std::string::npos && end != std::string::npos)
    {
        normalized = normalized.substr(start, end - start + 1);
    }

    std::transform(normalized.begin(), normalized.end(), normalized.begin(), ::tolower);

    return normalized;
}

void Blocklist::load_from_file(const std::string &filename)
{
    std::ifstream file(filename);

    if (!file.is_open())
    {
        std::cerr << "unable to open filter file:" + filename << std::endl;
        exit(1);
        return;
    }

    std::string line;
    while (std::getline(file, line))
    {
        // Skip comments and empty lines
        if (line.empty() || line[0] == '#')
        {
            continue;
        }

        std::string normalized = normalize_domain(line);

        if (!normalized.empty())
        {
            blocked_domains.insert(normalized);
        }
    }

    file.close();
}

bool Blocklist::is_blocked(const std::string &domain) const
{
    std::string normalized = normalize_domain(domain);

    if (blocked_domains.count(normalized) > 0)
    {
        return true;
    }

    // subdomain checking
    size_t pos = normalized.find('.');
    while (pos != std::string::npos)
    {
        std::string parent = normalized.substr(pos + 1);

        if (blocked_domains.count(parent) > 0)
        {
            return true;
        }

        pos = normalized.find('.', pos + 1);
    }

    return false;
}