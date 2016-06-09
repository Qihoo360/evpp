#pragma once

#include "evpp/inner_pre.h"
#include "evpp/event_loop.h"
#include "evpp/event_loop_thread_pool.h"

#include <atomic>

namespace evpp {
    class Listener;

    class EVPP_EXPORT TCPServer {
    public:
        TCPServer(EventLoop* loop,
                  const std::string& listen_addr,
                  const std::string& name,
                  int thread_num);

        bool Start();

    private:
        EventLoop* loop_;  // the acceptor loop
        const std::string listen_addr_;
        const std::string name_;
        xstd::shared_ptr<Listener> acceptor_; // avoid revealing Acceptor
        xstd::shared_ptr<EventLoopThreadPool> tpool_;
//         ConnectionCallback connectionCallback_;
//         MessageCallback messageCallback_;
//         WriteCompleteCallback writeCompleteCallback_;
//         ThreadInitCallback threadInitCallback_;
        std::atomic<int> started_;
        // always in loop thread
        int nextConnId_;
        //ConnectionMap connections_;
    };
}