#include "cache.hpp"
#include <map>
#include <chrono>
#include <iostream>

std::map<std::string, CacheEntry> cache;

std::vector<resourceRecord>* getCached(const std::string& domain_name) {
    auto it = cache.find(domain_name);
    
    if (it == cache.end()) {
        return nullptr; 
    }
    
    time_t now;
    time(&now);
    
    time_t elapsed = now - it->second.cache_time;
    uint32_t ttl = it->second.records[0].rr_ttl;
    
    if (elapsed >= ttl) {
        cache.erase(it);
        return nullptr;
    }
    
    return &it->second.records;
}

void addToCache(const std::vector<resourceRecord>& records) {
    if (records.empty()) return;
    
    const std::string& domain_name = records[0].name;
    time_t now;
    time(&now);
    
    cache[domain_name].records = records;
    cache[domain_name].cache_time = now;
}

void cleanupCache() {
    time_t now;
    time(&now);
    
    auto it = cache.begin();
    while (it != cache.end()) {
        time_t elapsed = now - it->second.cache_time;
        uint32_t ttl = it->second.records[0].rr_ttl;
        
        if (elapsed >= ttl) {
            it = cache.erase(it);
        } else {
            ++it;
        }
    }
}