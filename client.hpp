#include <vector>
#include <map>
#include <string> 
#include <iostream>

template <typename T>
uint8_t* to_bits(T value){
    return reinterpret_cast<uint8_t*>(value);
}

void create_question(std::string domainName, std::vector<uint8_t> &buffer);

void print_map(std::string_view comment, const std::map<std::string, int>& m);

// for each name in the message
// for each suffix of the name 
// for each suffix in the message
// if suffix max