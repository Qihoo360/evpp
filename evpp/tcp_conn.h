#pragma once

#include "evpp/inner_pre.h"
#include "evpp/buffer.h"
#include "evpp/tcp_callbacks.h"
#include "evpp/slice.h"
#include "evpp/any.h"

namespace evpp {

class EventLoop;
class FdChannel;
class TCPClient;

class EVPP_EXPORT TCPConn : public std::enable_shared_from_this<TCPConn> {
public:
    enum Type {
        kIncoming = 0, // The type of a TCPConn held by a TCPServer
        kOutgoing = 1, // The type of a TCPConn held by a TCPClient
    };
    enum Status {
        kDisconnected = 0,
        kConnecting = 1,
        kConnected = 2,
        kDisconnecting = 3,
    };
public:
    TCPConn(EventLoop* loop,
            const std::string& name,
            int sockfd,
            const std::string& laddr,
            const std::string& raddr);
    ~TCPConn();

    void Close();

    void Send(const char* s) {
        Send(s, strlen(s));
    }
    void Send(const void* d, size_t dlen);
    void Send(const std::string& d);
    void Send(const Slice& message);
    void Send(Buffer* buf);
public:
    EventLoop* loop() const {
        return loop_;
    }
    void set_context(const Any& c) {
        context_[0] = c;
    }
    const Any& context() const {
        return context_[0];
    }
    void set_context(int index, const Any& c) {
        assert(index < kContextCount && index >= 0);
        context_[index] = c;
    }
    const Any& context(int index) const {
        assert(index < kContextCount && index >= 0);
        return context_[index];
    }
    // Return the remote peer's address with form "ip:port"
    const std::string& remote_addr() const {
        return remote_addr_;
    }
    const std::string& name() const {
        return name_;
    }
    bool IsConnected() const {
        return status_ == kConnected;
    }
    bool IsConnecting() const {
        return status_ == kConnecting;
    }
    bool IsDisconnected() const {
        return status_ == kDisconnected;
    }
    bool IsDisconnecting() const {
        return status_ == kDisconnecting;
    }
    Type type() const {
        return type_;
    }
    Status status() const {
        return status_;
    }
    void SetCloseDelayTime(Duration d) {
        assert(type_ == kIncoming);
        // This option is only available for the connection type kIncoming
        // Set the delay time to close the socket
        close_delay_ = d;
    }
    void SetTCPNoDelay(bool on);

    // TODO Add : SetLinger();
protected:
    friend class TCPClient;
    friend class TCPServer;

    // These methods are visible only for TCPClient and TCPServer.
    // We don't want the user layer to access these methods.
    void set_type(Type t) {
        type_ = t;
    }
    void SetMessageCallback(MessageCallback cb) {
        msg_fn_ = cb;
    }
    void SetConnectionCallback(ConnectionCallback cb) {
        conn_fn_ = cb;
    }
    void SetHighWaterMarkCallback(const HighWaterMarkCallback& cb, size_t mark);
    void SetCloseCallback(CloseCallback cb) {
        close_fn_ = cb;
    }
    void OnAttachedToLoop();
private:
    void HandleRead();
    void HandleWrite();
    void HandleClose();
    void SendInLoop(const Slice& message);
    void SendInLoop(const void* data, size_t len);
    void SendStringInLoop(const std::string& message);
    std::string StatusToString() const;
private:
    EventLoop* loop_;
    int fd_;
    std::string name_;
    std::string local_addr_; // the local address with form : "ip:port"
    std::string remote_addr_; // the remote address with form : "ip:port"
    std::unique_ptr<FdChannel> chan_;
    Buffer input_buffer_;
    Buffer output_buffer_; // TODO use a list<Slice> ??

    enum { kContextCount = 16, };
    Any context_[kContextCount];
    Type type_;
    Status status_;
    size_t high_water_mark_; // Default 128MB

    // The delay time to close a incoming connection which has been shutdown by peer normally.
    // Default is 3 second.
    Duration close_delay_;

    ConnectionCallback conn_fn_; // This will be called to the user application layer
    MessageCallback msg_fn_; // This will be called to the user application layer
    WriteCompleteCallback write_complete_fn_; // This will be called to the user application layer
    HighWaterMarkCallback high_water_mark_fn_; // This will be called to the user application layer
    CloseCallback close_fn_; // This will be called to TCPClient or TCPServer
};
}