#pragma once

#include "evpp/inner_pre.h"
#include "evpp/event_loop.h"
#include "evpp/event_loop_thread_pool.h"
#include "evpp/tcp_callbacks.h"

#include <map>
#include <atomic>

namespace evpp {
    class Listener;

    class EVPP_EXPORT TCPServer {
    public:
        enum ThreadDispatchPolicy {
            kRoundRobin,
            kIPAddressHashing,
        };
        TCPServer(EventLoop* loop,
                  const std::string& listen_addr/*ip:port*/,
                  const std::string& name,
                  int thread_num);

        bool Start();
        void SetMesageHandler(MessageCallback cb) {
            messageCallback_ = cb;
        }
        void SetThreadDispatchPolicy(ThreadDispatchPolicy v) {
            threads_dispatch_policy_ = v;
        }
    private:
        void RemoveConnection(const TCPConnPtr& conn);
        void RemoveConnectionInLoop(const TCPConnPtr& conn); // Remove the connection in the listener EventLoop
        void HandleNewConn(int sockfd, const std::string& remote_addr/*ip:port*/);
        EventLoop* GetNextLoop(const std::string& remote_addr);
    private:
        typedef std::map<std::string, TCPConnPtr> ConnectionMap;

        EventLoop* loop_;  // the acceptor loop
        const std::string listen_addr_;
        const std::string name_;
        xstd::shared_ptr<Listener> listener_; // avoid revealing Acceptor
        xstd::shared_ptr<EventLoopThreadPool> tpool_;
        MessageCallback messageCallback_;
        ConnectionCallback connectionCallback_;
        WriteCompleteCallback writeCompleteCallback_;

        ThreadDispatchPolicy threads_dispatch_policy_;

        // always in loop thread
        uint64_t nextConnId_;
        ConnectionMap connections_; // TODO 这个变量真的有必要存在么？
    };
}