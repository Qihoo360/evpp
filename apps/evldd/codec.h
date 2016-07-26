#pragma once

#include "evpp/buffer.h"
#include "evpp/tcp_conn.h"
#include "evpp/timestamp.h"

namespace evldd {

class Codec {
public:
    Codec();
    void OnCodecMessage(const evpp::TCPConnPtr& conn,
                        evpp::Buffer* buf,
                        evpp::Timestamp ts);

}

}//namespace evldd