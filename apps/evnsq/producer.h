#pragma once

#include "client.h"

namespace evnsq {
    class EVNSQ_EXPORT Producer : public Client {
    public:
        Producer(evpp::EventLoop* loop, const std::string& topic, const std::string& channel, const Option& ops);
        ~Producer();
    
        void Publish(const std::string& msg);
    private:
        std::map<std::string/*host:port*/, ConnPtr>::iterator conn_;
    };
}

