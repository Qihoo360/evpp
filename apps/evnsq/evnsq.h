#pragma once

#include <string>
#include <memory>

#include <evpp/exp.h>
#include <evpp/tcp_callbacks.h>

#include "evnsq_export.h"

namespace evpp {
    class EventLoop;
    class TCPClient;
}

namespace evnsq {
    class EVNSQ_EXPORT NSQClient {
    public:
        NSQClient(evpp::EventLoop* loop, const std::string& raddr);
        ~NSQClient();
        bool Connect();
    private:
        void OnConnection(const evpp::TCPConnPtr& conn);
    private:
        evpp::EventLoop* loop_;
        std::shared_ptr<evpp::TCPClient> tcpc_;
    };
}