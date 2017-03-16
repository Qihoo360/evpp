#pragma once


#pragma pack(push,1)
struct Header {
    uint32_t body_size_; // net order
    uint32_t packet_count_; // net order

    uint32_t get_body_size() const {
        return ntohl(body_size_);
    }
    uint32_t get_full_size() const {
        return sizeof(Header) + ntohl(body_size_);
    }
    void inc_packet_count() {
        uint32_t count = ntohl(packet_count_) + 1;
        packet_count_ = htonl(count);
    }
};
#pragma pack(pop)

const size_t kMaxSize = 10000;