#pragma once

#include <string>
#include <memory>
#include <atomic>
#include <map>

#include <evpp/exp.h>
#include <evpp/tcp_callbacks.h>

#include "evnsq_export.h"

#include "client.h"

namespace evnsq {
    class Command;

    class EVNSQ_EXPORT Consumer : public Client {
    public:
        Consumer(evpp::EventLoop* loop, const std::string& topic, const std::string& channel, const Option& ops);
        ~Consumer();
    };
}

