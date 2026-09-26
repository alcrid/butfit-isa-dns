#ifndef DNS_MESSAGE_H
#define DNS_MESSAGE_H

#include <string>
#include <vector>
#include <cstdint>
#include <iostream>
#include <bitset>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>

struct resource_record
{
    std::string name;
    uint16_t rr_type;
    uint16_t rr_class;
    uint32_t rr_ttl;
    uint16_t rr_rdlength;
    std::vector<uint8_t> rdata;
};

struct dns_question
{
    std::string domain_name;
    uint16_t qtype;
    uint16_t qclass;
};

struct dns_header
{
    uint16_t id;
    uint16_t flags;
    uint16_t qdcount;
    uint16_t ancount;
    uint16_t nscount;
    uint16_t arcount;
};

struct dns_a_message
{
    dns_header header;
    std::vector<dns_question> questions;
    std::vector<resource_record> answers;
};

enum error_type{
    NO_ERROR,
    FORMAT_ERROR,
    SERVER_FAILURE,
    NAME_ERROR,
    NOT_IMPLEMENTED,
    REFUSED
};

uint8_t get8bits(std::vector<uint8_t> &buffer, size_t &offset);
uint16_t get16bits(std::vector<uint8_t> &buffer, size_t &offset);
uint32_t get32bits(std::vector<uint8_t> &buffer, size_t &offset);
std::string get_domain_name(std::vector<uint8_t> &buffer, size_t &offset);
resource_record get_resource_record(std::vector<uint8_t> &buffer, size_t &offset);
error_type parse_dns_message(std::vector<uint8_t> &buffer, std::string &domain_name);
error_type check_flags(std::vector<uint8_t> flags);

void create_question(std::vector<std::string> domain_names, std::vector<uint8_t> &buffer);
void create_dns_packet(uint16_t query_id, std::vector<uint8_t> &buffer, std::vector<std::string> domain_names);
void create_error_packet(std::vector<uint8_t>& packet, error_type error);
#endif