#include "dns_message.hpp"
#include "logger.hpp"

uint8_t get8bits(std::vector<uint8_t> &buffer, size_t &offset)
{
    if (offset + 1 > buffer.size())
    {
        throw std::out_of_range("Buffer overflow in get8bits");
    }
    return buffer[offset++];
}

uint16_t get16bits(std::vector<uint8_t> &buffer, size_t &offset)
{
    if (offset + 2 > buffer.size())
    {
        throw std::out_of_range("Buffer overflow in get16bits");
    }
    uint16_t value = (static_cast<uint16_t>(buffer[offset]) << 8) |
                     static_cast<uint16_t>(buffer[offset + 1]);
    offset += 2;
    return value;
}

uint32_t get32bits(std::vector<uint8_t> &buffer, size_t &offset)
{
    if (offset + 4 > buffer.size())
    {
        throw std::out_of_range("Buffer overflow in get32bits");
    }
    uint32_t value = (static_cast<uint32_t>(buffer[offset]) << 24) |
                     (static_cast<uint32_t>(buffer[offset + 1]) << 16) |
                     (static_cast<uint32_t>(buffer[offset + 2]) << 8) |
                     (static_cast<uint32_t>(buffer[offset + 3]));
    offset += 4;
    return value;
}

std::string get_domain_name(std::vector<uint8_t> &buffer, size_t &offset)
{
    std::string domain_name;
    bool first = true;
    while (offset < buffer.size())
    {
        uint8_t length = buffer[offset++];

        if (length == 0)
        {
            return domain_name;
        }

        if ((length & 0xC0) == 0xC0)
        {
            uint16_t pointer_offset = ((length & 0x3F) << 8) | buffer[offset++];
            // check if the pointer it out of bounds
            if (pointer_offset >= buffer.size())
            {
                throw std::out_of_range("Pointer outside of message bounds");
            }

            // check if the poitner isn't pointing to the header
            if (pointer_offset < 12) {
                throw std::out_of_range("Compression pointer into DNS header");
            }

            size_t old_offset = offset;
            offset = pointer_offset;
            std::string pointed_name = get_domain_name(buffer, offset);
            offset = old_offset;

            if (!first)
            {
                domain_name += '.';
            }
            domain_name += pointed_name;

            return domain_name;
        }
        if (!first)
        {
            domain_name += '.';
        }
        first = false;

        for (uint8_t pos = 0; pos < length; pos++)
        {
            domain_name += static_cast<char>(buffer[offset++]);
        }
    }
    return domain_name;
}

error_type check_flags(uint16_t flags)
{
    if ((flags >> 15) & 0x1)
    {
        Logger::log("Error: QR bit set (response, not query)");
        return FORMAT_ERROR;
    }

    uint8_t opcode = (flags >> 11) & 0xF;
    if (opcode != 0)
    {
        Logger::log("Error: Unsupported OPCODE: " + std::to_string(opcode));
        return NOT_IMPLEMENTED;
    }

    if ((flags >> 9) & 0x1)
    {
        Logger::log("Warning: TC bit set in query");
        return FORMAT_ERROR;
    }

    return NO_ERROR;
}

error_type parse_dns_message(std::vector<uint8_t> &buffer, std::string &domain_name)
{
    if (buffer.size() < 12)
    {
        Logger::log("Packet too small for DNS header");
        return FORMAT_ERROR;
    }

    try
    {
        size_t offset = 0;
        dns_a_message message;

        message.header.id = get16bits(buffer, offset);
        message.header.flags = get16bits(buffer, offset);
        error_type flag_error;
        if ((flag_error = check_flags(message.header.flags)) != NO_ERROR)
        {
            return flag_error;
        }

        message.header.qdcount = get16bits(buffer, offset);
        message.header.ancount = get16bits(buffer, offset);
        message.header.nscount = get16bits(buffer, offset);
        message.header.arcount = get16bits(buffer, offset);

        Logger::log("=== DNS Header ===");
        Logger::log("id: " + std::to_string(message.header.id));
        Logger::log("flags: " + std::bitset<16>(message.header.flags).to_string());
        Logger::log("qdcount: " + std::to_string(message.header.qdcount));
        Logger::log("ancount: " + std::to_string(message.header.ancount));
        Logger::log("nscount: " + std::to_string(message.header.nscount));
        Logger::log("arcount: " + std::to_string(message.header.arcount));

        dns_question question;
        for (int i = 0; i < message.header.qdcount; i++)
        {
            question.domain_name = get_domain_name(buffer, offset);

            question.qtype = get16bits(buffer, offset);
            question.qclass = get16bits(buffer, offset);
            if (question.qclass != 0x0001)
            {
                Logger::log("Unsupported qclass detected");
                return NOT_IMPLEMENTED;
            }
            if (question.qtype != 0x0001)
            {
                Logger::log("Unsupported qtype detected");
                return NOT_IMPLEMENTED;
            }

            if (i == 0)
            {
                domain_name = question.domain_name;
            }
            message.questions.push_back(question);
        }
    }
    catch (const std::out_of_range &e)
    {
        Logger::log("Error: " + std::string(e.what()));
        return FORMAT_ERROR;
    }
    return NO_ERROR;
}

void create_question(std::vector<std::string> domain_names, std::vector<uint8_t> &buffer)
{
    for (const std::string &domain_name : domain_names)
    {
        std::string label;
        size_t pos = 0;
        size_t start = 0;
        while ((pos = domain_name.find('.', start)) != std::string::npos)
        {
            label = domain_name.substr(start, pos - start);
            buffer.push_back(label.length());
            buffer.insert(buffer.end(), label.begin(), label.end());
            start = pos + 1;
        }

        label = domain_name.substr(start);
        if (!label.empty())
        {
            buffer.push_back(label.length());
            buffer.insert(buffer.end(), label.begin(), label.end());
        }

        uint16_t qtype = htons(1);
        uint16_t qclass = htons(1);

        uint8_t *qtype_bits = reinterpret_cast<uint8_t *>(&qtype);
        uint8_t *qclass_bits = reinterpret_cast<uint8_t *>(&qclass);
        buffer.push_back(0);

        buffer.insert(buffer.end(), qtype_bits, qtype_bits + sizeof(qtype));
        buffer.insert(buffer.end(), qclass_bits, qclass_bits + sizeof(qclass));
    }
}

void create_dns_packet(uint16_t query_id, std::vector<uint8_t> &buffer, std::vector<std::string> domain_names)
{
    dns_header header;
    header.id = htons(query_id);
    header.flags = htons(0x0100);
    header.qdcount = htons(domain_names.size());
    header.ancount = 0;
    header.nscount = 0;
    header.arcount = 0;

    uint8_t *header_bits = reinterpret_cast<uint8_t *>(&header);
    buffer.insert(buffer.end(), header_bits, header_bits + sizeof(header));
    create_question(domain_names, buffer);
}

void create_error_packet(std::vector<uint8_t> &packet, error_type error)
{
    // if the dns header is too small
    if (packet.size() < 12) {
        packet.resize(12, 0);
    }
    uint16_t flags = ntohs(*reinterpret_cast<uint16_t *>(&packet[2]));
    flags &= ~0x000F;
    flags |= (error & 0x000F);
    flags |= 0x8000;

    *reinterpret_cast<uint16_t *>(&packet[2]) = htons(flags);
}