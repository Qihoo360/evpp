#pragma once
namespace evldd {

class Packet {

public:

    MessagePtr RetriveMessage();

    void   Reset();
    size_t BodySize();

    bool IsOk();

    const Header&   header();
    const Body&   Body();

    size_t ReadHeader(void* data);
    size_t ReadBody(void* data);
}
}
