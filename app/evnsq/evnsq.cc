#include "evnsq.h"

#include <evpp/event_loop.h>
#include <evpp/tcp_client.h>

namespace evnsq {
    NSQClient::NSQClient(evpp::EventLoop* loop, const std::string& raddr) {

    }

    NSQClient::~NSQClient() {

    }

    bool NSQClient::Connect() {
        return false;
    }
}