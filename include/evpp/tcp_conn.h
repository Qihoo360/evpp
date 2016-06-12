#pragma once

#include "evpp/inner_pre.h"
#include "evpp/buffer.h"
#include "evpp/tcp_callbacks.h"
#include "evpp/slice.h"

namespace evpp {

    class EventLoop;
    class FdChannel;
    class TCPClient;

    class EVPP_EXPORT TCPConn : public xstd::enable_shared_from_this<TCPConn> {
    public:
        enum Type {
            kIncoming, // The type of a TCPConn held by a TCPServer
            kOutgoing, // The type of a TCPConn held by a TCPClient
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

        void Send(const char* s) { Send(s, strlen(s)); }
        void Send(const void* d, size_t dlen);
        void Send(const std::string& d);
        void Send(const Slice& message);
        void Send(Buffer* buf);
        void Close();
        void OnAttachedToLoop();
    public:
        void SetMesageHandler(MessageCallback cb) { msg_fn_ = cb; }
        void SetConnectionHandler(ConnectionCallback cb) { conn_fn_ = cb; }
        void SetCloseCallback(CloseCallback cb) { close_fn_ = cb; }
        void SetHighWaterMarkCallback(const HighWaterMarkCallback& cb, size_t mark);
    public:
        const std::string& remote_addr() const { return remote_addr_; }
        const std::string& name() const { return name_; }
        bool IsConnected() const { return status_ == kConnected; }
        bool IsDisconnected() const { return status_ == kDisconnected; }
        Type type() const { return type_; }
        void set_type(Type t) { type_ = t; }
        Status status() const { return status_; }
        void set_status(Status s) { status_ = s; }
    private:
        void HandleRead(Timestamp receiveTime);
        void HandleWrite();
        void HandleClose();
        void HandleError();
        void SendInLoop(const Slice& message);
        void SendInLoop(const void* data, size_t len);
        void SendStringInLoop(const std::string& message);
    private:
//         friend class TCPClient;
//         void set_local_addr(const std::string& addr) { local_addr_ = addr; }
//         void set_name(const std::string& n) { name_ = n; }
//         xstd::shared_ptr<FdChannel> chan() { return chan_; }
    private:
        EventLoop* loop_;
        int fd_;
        std::string name_;
        std::string local_addr_; // the local address of ip:port
        std::string remote_addr_; // the remote address of  ip:port
        xstd::shared_ptr<FdChannel> chan_;
        Buffer input_buffer_;
        Buffer output_buffer_;

        Type type_;
        Status status_;
        size_t high_water_mark_;

        ConnectionCallback conn_fn_;
        MessageCallback msg_fn_;
        WriteCompleteCallback write_complete_fn_;
        HighWaterMarkCallback high_water_mark_fn_;
        CloseCallback close_fn_;
    };
}