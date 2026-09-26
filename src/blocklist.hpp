#ifndef BLOCKLIST_H
#define BLOCKLIST_H

#include <string>
#include <unordered_set>
class Blocklist
{
public:
    void load_from_file(const std::string &filename);

    bool is_blocked(const std::string &domain) const;

private:
    std::unordered_set<std::string> blocked_domains;
    std::string normalize_domain(const std::string &domain) const;
};

#endif