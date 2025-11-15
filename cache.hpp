#include <map>
#include <vector>
#include <string>
#include <ctime>
#include <cstdint>
#include "dns_message.hpp"

struct CacheEntry {
    std::vector<DnsMessage> records;
    time_t cache_time;
};

extern std::map<std::string, CacheEntry> cache;

std::vector<resourceRecord>* getCached(const std::string& domain_name);

void addToCache(const std::vector<resourceRecord>& records);

void cleanupCache();