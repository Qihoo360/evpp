#include "evpp/inner_pre.h"

#include "evpp/libevent_headers.h"

#include "evpp/tcp_conn.h"
#include "evpp/fd_channel.h"
#include "evpp/event_loop.h"

namespace evpp {
    TCPConn::TCPConn(EventLoop* loop,
                     const std::string& n,
                     int sockfd,
                     const std::string& laddr,
                     const std::string& raddr)
                     : loop_(loop)
                     , fd_(sockfd)
                     , name_(n)
                     , local_addr_(laddr)
                     , remote_addr_(raddr) {
        chan_.reset(new FdChannel(loop, sockfd, false, false));

        chan_->SetReadCallback(xstd::bind(&TCPConn::HandleRead, this, xstd::placeholders::_1));
        chan_->SetWriteCallback(xstd::bind(&TCPConn::HandleWrite, this));
        chan_->SetCloseCallback(xstd::bind(&TCPConn::HandleClose, this));
        chan_->SetErrorCallback(xstd::bind(&TCPConn::HandleError, this));
        LOG_DEBUG << "TCPConn::[" << name_ << "] at " << this << " fd=" << sockfd;
        //TODO  set KeepAlive;
    }

    TCPConn::~TCPConn() {
        LOG_INFO << "TCPConn::~TCPConn() close(fd=" << fd_ << ")";
        EVUTIL_CLOSESOCKET(fd_);
        fd_ = INVALID_SOCKET;
    }

    void TCPConn::HandleRead(base::Timestamp receiveTime) {
        loop_->AssertInLoopThread();
        int serrno = 0;
        ssize_t n = input_buffer_.ReadFromFD(chan_->fd(), &serrno);
        if (n > 0) {
            msg_fn_(shared_from_this(), &input_buffer_, receiveTime);
        } else if (n == 0) {
            HandleClose();
        } else {
            LOG_ERROR << "TCPConn::HandleRead errno=" << serrno << " " << strerror(serrno);
            if (serrno != EAGAIN && serrno != EINTR) {
                HandleClose();
            } else {
                HandleError();
            }
        }
    }

    void TCPConn::HandleWrite() {
        loop_->AssertInLoopThread();
        //TODO
    }

    void TCPConn::HandleClose() {
        loop_->AssertInLoopThread();
        chan_->DisableAllEvent();
        chan_->Close();

        TCPConnPtr conn(shared_from_this());
        if (conn_fn_) {
            conn_fn_(conn);
        }
        close_fn_(conn);// This must be the last line
    }

    void TCPConn::HandleError() {
    }

    void TCPConn::Send(const void* d, size_t dlen) {
        //TODO handle write error, handle the case that it is not the same IO thread
        ::send(chan_->fd(), (const char*)d, dlen, 0);
    }

    void TCPConn::OnAttachedToLoop() {
        loop_->AssertInLoopThread();
        chan_->AttachToLoop();
        chan_->EnableReadEvent();
    }
}
