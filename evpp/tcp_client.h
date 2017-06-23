#pragma once

#include "evpp/inner_pre.h"
#include "evpp/event_loop.h"
#include "evpp/tcp_callbacks.h"
#include "evpp/any.h"

#include <map>
#include <atomic>
#include <mutex>

namespace evpp {
class Connector;

// We can use this class to create a TCP client.
// The typical usage is :
//      1. Create a TCPClient object
//      2. Set the message callback and connection callback
//      3. Call TCPClient::Connect() to try to establish a connection with remote server
//      4. Use TCPClient::Send(...) to send messages to remote server
//      5. Handle the connection and messages in callbacks
//      6. Call TCPClient::Disonnect() to disconnect from remote server
//
class EVPP_EXPORT TCPClient {
public:
    // @brief The constructor of the class
    // @param[in] loop - The EventLoop runs this object
    // @param[in] remote_addr - The remote server address with format "host:port"
    //  If the host is not IP, it will automatically do the DNS resolving asynchronously
    // @param[in] name -
    TCPClient(EventLoop* loop,
              const std::string& remote_addr/*host:port*/,
              const std::string& name);
    ~TCPClient();

    // @brief We can bind a local address. This is an optional operation.
    //  If necessary, it should be called before doing Connect().
    // @param[IN] local_addr -
    void Bind(const std::string& local_addr/*host:port*/);

    // @brief Try to establish a connection with remote server asynchronously
    //  If the connection callback is set properly it will be invoked when
    //  the connection is established successfully or timeout or cannot
    //  establish a connection.
    void Connect();

    // @brief Disconnect from the remote server. When the connection is
    //  broken down, the connection callback will be invoked.
    void Disconnect();
public:
    // Set a connection event relative callback when the TCPClient
    // establishes a connection or an exist connection breaks down or failed to establish a connection.
    // When these three events happened, the value of the parameter in the callback is:
    //      1. Successfully establish a connection : TCPConn::IsConnected() == true
    //      2. An exist connection broken down : TCPConn::IsDisconnecting() == true
    //      3. Failed to establish a connection : TCPConn::IsDisconnected() == true and TCPConn::fd() == -1
    void SetConnectionCallback(const ConnectionCallback& cb);

    // Set the message callback to handle the messages from remote server
    void SetMessageCallback(const MessageCallback& cb) {
        msg_fn_ = cb;
    }

public:
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
    const std::string& local_addr() const {
        return local_addr_;
    }
    const std::string& name() const {
        return name_;
    }
    EventLoop* loop() const {
        return loop_;
    }
private:
    void DisconnectInLoop();
    void OnConnection(evpp_socket_t sockfd, const std::string& laddr);
    void OnRemoveConnection(const TCPConnPtr& conn);
    void Reconnect();
private:
    EventLoop* loop_;
    std::string local_addr_; // If the local address is not empty, we will bind to this local address before doing connect()
    std::string remote_addr_; // host:port
    std::string name_;
    std::atomic<bool> auto_reconnect_ = { true }; // The flag whether it reconnects automatically, Default : true
    Duration reconnect_interval_ = Duration(3.0); // Default : 3 seconds

    Any context_;

    mutable std::mutex mutex_; // The guard of conn_
    TCPConnPtr conn_;

    std::shared_ptr<Connector> connector_;
    Duration connecting_timeout_ = Duration(3.0); // Default : 3 seconds

    ConnectionCallback conn_fn_;
    MessageCallback msg_fn_;
};
}
