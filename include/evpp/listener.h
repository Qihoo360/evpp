#pragma once

#include "evpp/inner_pre.h"

namespace evpp {
    class EventLoop;
    class FdChannel;

    class Listener {
    public:
        Listener(EventLoop* loop, const std::string& addr);
        ~Listener();

        void Start();
        //void SetNewConnectionCallback();
        static int CreateNonblockingSocket();
    private:
        
    private:
        int fd_;// The listener socket fd
        xstd::shared_ptr<FdChannel> chan_;
        EventLoop* loop_;
        std::string addr_;
    };
}


