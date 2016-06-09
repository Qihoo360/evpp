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
                const std::string& local_addr,
                const std::string& peer_addr);
        ~TCPConn();

        void Send(const void* d, size_t dlen);

        const std::string& remote_addr() const {
            return remote_addr_;
        }

        const std::string& name() const {
            return name_;
        }

        void OnAttachedToLoop();
    public:
        void SetMesageHandler(MessageCallback cb) {
            messageCallback_ = cb;
        }
        void SetConnectionHandler(ConnectionCallback cb) {
            connectionCallback_ = cb;
        }
        void SetCloseCallback(CloseCallback cb) {
            closeCallback_ = cb;
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
        std::string local_addr_; // ip:port
        std::string remote_addr_; // ip:port
        xstd::shared_ptr<FdChannel> chan_;
        Buffer inputBuffer_;
        Buffer outputBuffer_;

        ConnectionCallback connectionCallback_;
        MessageCallback messageCallback_;
        WriteCompleteCallback writeCompleteCallback_;
        HighWaterMarkCallback highWaterMarkCallback_;
        CloseCallback closeCallback_;
    };
}