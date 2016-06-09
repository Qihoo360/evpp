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
        void SetMesageHandler(MessageCallback cb) {
            messageCallback_ = cb;
        }

        void Send(const void* d, size_t dlen);

        // returns the remote network address.
        const std::string& remote_addr() const {
            return remote_addr_;
        }

        void OnAttachedToLoop();
    private:
        void HandleRead(base::Timestamp receiveTime);
        void HandleWrite();
        void HandleClose();
        void HandleError();
    private:
        EventLoop* loop_;
        std::string name_;
        std::string local_addr_;
        std::string remote_addr_;
        xstd::shared_ptr<FdChannel> chan_;
        Buffer inputBuffer_;
        Buffer outputBuffer_;

        ConnectionCallback connectionCallback_;
        MessageCallback messageCallback_;
        WriteCompleteCallback writeCompleteCallback_;
        HighWaterMarkCallback highWaterMarkCallback_;
    };
}