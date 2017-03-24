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
    TCPClient(EventLoop* loop, const std::string& remote_addr/*host:port*/, const std::string& name);
    ~TCPClient();
    void Connect();
    void Disconnect();
public:
    // Set a connection event relative callback when the TCPClient
    // establishes a connection or an exist connection breaks down or failed to establish a connection.
    // When these three events happened, the value of the parameter in the callback is:
    //      1. Successfully establish a connection : TCPConn::IsConnected() == true
    //      2. An exist connection broken down : TCPConn::IsDisconnecting() == true
    //      3. Failed to establish a connection : TCPConn::IsDisconnected() == true and TCPConn::fd() == -1
    void SetConnectionCallback(const ConnectionCallback& cb);

    void SetMessageCallback(const MessageCallback& cb) {
        msg_fn_ = cb;
    }
    bool auto_reconnect() const {
        return auto_reconnect_;
    }
    void set_auto_reconnect(bool v) {
        auto_reconnect_.store(v);
    }
    Duration reconnect_interval() const {
        return reconnect_interval_;
    }
    void set_reconnect_interval(Duration timeout) {
        reconnect_interval_ = timeout;
    }
    Duration connecting_timeout() const {
        return connecting_timeout_;
    }
    void set_connecting_timeout(Duration timeout) {
        connecting_timeout_ = timeout;
    }
    void set_context(const Any& c) {
        context_ = c;
    }
    const Any& context() const {
        return context_;
    }
    TCPConnPtr conn() const;

    // Return the remote address with the format of 'host:port'
    const std::string& remote_addr() const {
        return remote_addr_;
    }
    const std::string& name() const {
        return name_;
    }
    EventLoop* event_loop() const {
        return loop_;
    }
private:
    void DisconnectInLoop();
    void OnConnection(int sockfd, const std::string& laddr);
    void OnRemoveConnection(const TCPConnPtr& conn);
    void Reconnect();
private:
    EventLoop* loop_;
    std::string remote_addr_; // host:port
    std::string name_;
    std::atomic<bool> auto_reconnect_; // The flag whether it reconnects automatically, Default : true
    Duration reconnect_interval_; // Default : 3 seconds

    Any context_;

    mutable std::mutex mutex_; // The guard of conn_
    TCPConnPtr conn_;

    std::shared_ptr<Connector> connector_;
    Duration connecting_timeout_; // Default : 3 seconds

    ConnectionCallback conn_fn_;
    MessageCallback msg_fn_;
};
}
