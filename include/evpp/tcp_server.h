#pragma once

#include "evpp/inner_pre.h"
#include "evpp/event_loop.h"
#include "evpp/event_loop_thread_pool.h"
#include "evpp/tcp_callbacks.h"

#include <map>

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
        ~TCPServer();
        bool Start();
        void Stop();
        void SetMesageCallback(MessageCallback cb) {
            msg_fn_ = cb;
        }
        void SetThreadDispatchPolicy(ThreadDispatchPolicy v) {
            threads_dispatch_policy_ = v;
        }
    private:
        void StopInLoop();
        void RemoveConnection(const TCPConnPtr& conn);
        void RemoveConnectionInLoop(const TCPConnPtr& conn); // Remove the connection in the listener EventLoop
        void HandleNewConn(int sockfd, const std::string& remote_addr/*ip:port*/);
        EventLoop* GetNextLoop(const std::string& remote_addr);
    private:
        typedef std::map<std::string, TCPConnPtr> ConnectionMap;

        EventLoop* loop_;  // the listener loop
        const std::string listen_addr_;
        const std::string name_;
        std::shared_ptr<Listener> listener_;
        std::shared_ptr<EventLoopThreadPool> tpool_;
        MessageCallback msg_fn_;
        ConnectionCallback conn_fn_;
        WriteCompleteCallback write_complete_fn_;

        ThreadDispatchPolicy threads_dispatch_policy_;

        // always in loop thread
        uint64_t next_conn_id_;
        ConnectionMap connections_; // TODO 这个变量真的有必要存在么？由于每一个连接都会在libevent内部做好记录，这里再保存一份理论上是没必要的。但代码看起来更清晰一些。
    };
}