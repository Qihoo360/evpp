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
        close_delay_ = d;    // 设置延时关闭时间，仅对 kIncoming 类型的TCPConn生效。
    }
protected:
    // 这部分函数只能被 TCPClient 或者 TCPServer 调用，
    // 我们不希望上层应用来调用这些函数
    friend class TCPClient;
    friend class TCPServer;
    void set_type(Type t) {
        type_ = t;
    }
    void SetMessageCallback(MessageCallback cb) {
        msg_fn_ = cb;    // 会回调到上层应用
    }
    void SetConnectionCallback(ConnectionCallback cb) {
        conn_fn_ = cb;    // 会回调到上层应用
    }
    void SetHighWaterMarkCallback(const HighWaterMarkCallback& cb, size_t mark); // 会回调到上层应用
    void SetCloseCallback(CloseCallback cb) {
        close_fn_ = cb;    // 会回调到 TCPClient 或 TCPServer
    }
    void OnAttachedToLoop();
private:
    void HandleRead(Timestamp recv_time);
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
    std::string local_addr_; // the local address of ip:port
    std::string remote_addr_; // the remote address of ip:port
    std::unique_ptr<FdChannel> chan_;
    Buffer input_buffer_;
    Buffer output_buffer_;

    enum { kContextCount = 16, };
    Any context_[kContextCount];
    Type type_;
    Status status_;
    size_t high_water_mark_; // Default 128MB

    // The delay time to close a incoming connection which has been shutdown by peer normally.
    // Default is 3 second.
    Duration close_delay_;

    ConnectionCallback conn_fn_;
    MessageCallback msg_fn_;
    WriteCompleteCallback write_complete_fn_;
    HighWaterMarkCallback high_water_mark_fn_;
    CloseCallback close_fn_;
};
}