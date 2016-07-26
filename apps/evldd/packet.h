#pragma once

#include "message.h"
#include "header.h"
#include "extras_header.h"

namespace evldd {

class IncomingPacket {

public:
    MessagePtr RetriveMessage();

    //packet_size - header_size
    size_t LeftSize();

    bool IsOk();

    const Header::Parser&       header();
    const ExtrasHeader::Parser& extras_header();
    const Body::Parser&         Body();
    const Extras::Parser&       Extras();

    size_t ReadHeader(const void* data);
    size_t ReadBody(const void* data);

private:
    evpp::Buffer            buffer_;
    //alway has header
    Header::Parser          header_parser_;
    Body::Parser            body_parser_;
    ExtrasHeader::Parser    extras_header_parser_;
    Extras::Parser          extras_parser_;

};

}
