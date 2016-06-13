#pragma once

#include "evpp/inner_pre.h"
#include "evpp/event_loop.h"
#include "evpp/event_loop_thread_pool.h"
#include "evpp/tcp_callbacks.h"

#include <map>
#include <atomic>

namespace evpp {
    class Connector;
    class EVPP_EXPORT TCPClient {
    public:
        TCPClient(EventLoop* loop, const std::string& remote_addr, const std::string& name);

        void Connect();
        void Disconnect();
    public:
        void SetConnectionCallback(const ConnectionCallback& cb) { conn_fn_ = cb; }
        void SetMesageHandler(const MessageCallback& cb) { msg_fn_ = cb; }
        void SetWriteCompleteCallback(const WriteCompleteCallback& cb) { write_complete_fn_ = cb; }
        void set_connection_timeout(Duration timeout) { connection_timeout_ = timeout; }
    private:
        void ConnectInLoop();
        void DisconnectInLoop();
        void OnConnection(int sockfd, const std::string& laddr);
        void OnRemoveConnection(const TCPConnPtr& conn);
        void Reconnect();
    private:
        EventLoop* loop_;
        std::string remote_addr_;
        std::string name_;
        std::atomic<int> auto_reconnect_;

        TCPConnPtr conn_;
        xstd::shared_ptr<Connector> connector_;
        Duration connection_timeout_; // Default : 3 seconds

        ConnectionCallback conn_fn_;
        MessageCallback msg_fn_;
        WriteCompleteCallback write_complete_fn_;
    };
}