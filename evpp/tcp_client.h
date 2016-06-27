#pragma once

#include "evpp/inner_pre.h"
#include "evpp/event_loop.h"
#include "evpp/event_loop_thread_pool.h"
#include "evpp/tcp_callbacks.h"
#include "evpp/any.h"

#include <map>
#include <atomic>
#include <mutex>

namespace evpp {
    class Connector;
    class EVPP_EXPORT TCPClient {
    public:
        //! \brief
        //! \param[in] - EventLoop * loop
        //! \param[in] - const std::string & remote_addr The remote server address of the form "host:port"
        //! \param[in] - const std::string & name
        //! \return - 
        TCPClient(EventLoop* loop, const std::string& remote_addr, const std::string& name);
        ~TCPClient();
        void Connect();
        void Disconnect();
    public:
        void SetConnectionCallback(const ConnectionCallback& cb) { conn_fn_ = cb; }
        void SetMessageCallback(const MessageCallback& cb) { msg_fn_ = cb; }
        void SetWriteCompleteCallback(const WriteCompleteCallback& cb) { write_complete_fn_ = cb; }
        void set_connection_timeout(Duration timeout) { connection_timeout_ = timeout; }
        void set_context(const Any& c) { context_ = c; }
        const Any& context() const { return context_; }
        TCPConnPtr conn() const;
        const std::string& remote_addr() const { return remote_addr_; }
        const std::string& name() const { return name_; }
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
        Any context_;

        mutable std::mutex mutex_; // The guard of conn_
        TCPConnPtr conn_;

        std::shared_ptr<Connector> connector_;
        Duration connection_timeout_; // Default : 3 seconds

        ConnectionCallback conn_fn_;
        MessageCallback msg_fn_;
        WriteCompleteCallback write_complete_fn_;
    };
}