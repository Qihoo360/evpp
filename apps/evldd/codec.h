#pragma once

#include "evpp/buffer.h"
#include "evpp/tcp_conn.h"
#include "evpp/timestamp.h"

namespace evldd {

class Server;
class ChannelBase;

class Codec {
    typedef std::shared_ptr<ChannelBase> ChannelPtr;
public:
    Codec();
    void OnCodecMessage(const evpp::TCPConnPtr& conn,
                        evpp::Buffer* buf,
                        evpp::Timestamp ts);
private:
    Server* owner_;

};

}//namespace evldd