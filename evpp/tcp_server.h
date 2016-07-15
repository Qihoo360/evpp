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

    // 设置一个连接相关的回调函数，当接收到一个新的连接、或已有连接断开等事件发生时，都会调用该回调
    //  当成功建立连接时，回调中的参数 TCPConn::IsConnected() == true
    //  当连接断开时，回调中的参数 TCPConn::IsDisconnecting() == true
    void SetConnectionCallback(const ConnectionCallback& cb) {
        conn_fn_ = cb;
    }

    void SetMessageCallback(MessageCallback cb) {
        msg_fn_ = cb;
    }

    void SetThreadDispatchPolicy(ThreadDispatchPolicy v) {
        threads_dispatch_policy_ = v;
    }
private:
    void StopInLoop();
    void RemoveConnection(const TCPConnPtr& conn);
    void RemoveConnectionInLoop(const TCPConnPtr& conn); // Remove the connection in the listener EventLoop
    void HandleNewConn(int sockfd, const std::string& remote_addr/*ip:port*/, const struct sockaddr_in* raddr);
    EventLoop* GetNextLoop(const struct sockaddr_in* raddr);
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