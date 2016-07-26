#pragma once

#include "message.h"

namespace evldd {

class Header {
};

class Body {
};

class Packet {

public:

    MessagePtr RetriveMessage();

    void   Reset();
    size_t BodySize();

    bool IsOk();

    const Header&   header();
    const Body&   Body();

    size_t ReadHeader(const void* data);
    size_t ReadBody(const void* data);
};

}
