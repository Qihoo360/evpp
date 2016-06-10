#pragma once

#include "evpp/inner_pre.h"
#include "evpp/buffer.h"
#include "evpp/tcp_callbacks.h"

namespace evpp {
    
    class EventLoop;
    class FdChannel;

    class EVPP_EXPORT TCPConn 
        : public xstd::enable_shared_from_this<TCPConn> {
    public:
        TCPConn(EventLoop* loop,
                const std::string& name,
                int sockfd,
                const std::string& laddr,
                const std::string& raddr);
        ~TCPConn();

        void Send(const void* d, size_t dlen);
        
        void OnAttachedToLoop();
    public:
        void SetMesageHandler(MessageCallback cb) {
            msg_fn_ = cb;
        }
        void SetConnectionHandler(ConnectionCallback cb) {
            conn_fn_ = cb;
        }
        void SetCloseCallback(CloseCallback cb) {
            close_fn_ = cb;
        }

    public:
        const std::string& remote_addr() const {
            return remote_addr_;
        }

        const std::string& name() const {
            return name_;
        }
    private:
        void HandleRead(base::Timestamp receiveTime);
        void HandleWrite();
        void HandleClose();
        void HandleError();
    private:
        EventLoop* loop_;
        int fd_;
        std::string name_;
        std::string local_addr_; // the local address of ip:port
        std::string remote_addr_; // the remote address of  ip:port
        xstd::shared_ptr<FdChannel> chan_;
        Buffer input_buffer_;
        Buffer output_buffer_;

        ConnectionCallback conn_fn_;
        MessageCallback msg_fn_;
        WriteCompleteCallback write_fn_;
        HighWaterMarkCallback high_water_mark_fn_;
        CloseCallback close_fn_;
    };
}