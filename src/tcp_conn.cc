#include "evpp/inner_pre.h"

#include "evpp/libevent_headers.h"

#include "evpp/tcp_conn.h"
#include "evpp/fd_channel.h"
#include "evpp/event_loop.h"

namespace evpp {
    TCPConn::TCPConn(EventLoop* loop,
                     const std::string& name,
                     int sockfd,
                     const std::string& local_addr,
                     const std::string& peer_addr)
                     : loop_(loop)
                     , fd_(sockfd)
                     , name_(name)
                     , local_addr_(local_addr)
                     , remote_addr_(peer_addr) {
        chan_.reset(new FdChannel(loop, sockfd, false, false));

        chan_->SetReadCallback(
            xstd::bind(&TCPConn::HandleRead, this, xstd::placeholders::_1));
        chan_->SetWriteCallback(
            xstd::bind(&TCPConn::HandleWrite, this));
        chan_->SetCloseCallback(
            xstd::bind(&TCPConn::HandleClose, this));
        chan_->SetErrorCallback(
            xstd::bind(&TCPConn::HandleError, this));
        LOG_DEBUG << "TcpConnection::ctor[" << name_ << "] at " << this
            << " fd=" << sockfd;
        //socket_->setKeepAlive(true);
    }

    TCPConn::~TCPConn() {
        LOG_INFO << "TCPConn::~TCPConn() close(fd=" << fd_ << ")";
        EVUTIL_CLOSESOCKET(fd_);
        fd_ = INVALID_SOCKET;
    }

    void TCPConn::HandleRead(base::Timestamp receiveTime) {
        loop_->AssertInLoopThread();
        int serrno = 0;
        ssize_t n = inputBuffer_.ReadFromFD(chan_->fd(), &serrno);
        if (n > 0) {
            messageCallback_(shared_from_this(), &inputBuffer_, receiveTime);
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
        //         if (channel_->isWriting()) {
        //             ssize_t n = sockets::write(channel_->fd(),
        //                                        outputBuffer_.peek(),
        //                                        outputBuffer_.readableBytes());
        //             if (n > 0) {
        //                 outputBuffer_.retrieve(n);
        //                 if (outputBuffer_.readableBytes() == 0) {
        //                     channel_->disableWriting();
        //                     if (writeCompleteCallback_) {
        //                         loop_->queueInLoop(boost::bind(writeCompleteCallback_, shared_from_this()));
        //                     }
        //                     if (state_ == kDisconnecting) {
        //                         shutdownInLoop();
        //                     }
        //                 }
        //             } else {
        //                 LOG_SYSERR << "TcpConnection::handleWrite";
        //                 // if (state_ == kDisconnecting)
        //                 // {
        //                 //   shutdownInLoop();
        //                 // }
        //             }
        //         } else {
        //             LOG_TRACE << "Connection fd = " << channel_->fd()
        //                 << " is down, no more writing";
        //         }
    }

    void TCPConn::HandleClose() {
        loop_->AssertInLoopThread();
        chan_->DisableAllEvent();
        chan_->Close();

        TcpConnectionPtr conn(shared_from_this());
        if (connectionCallback_) {
            connectionCallback_(conn);
        }
        closeCallback_(conn);// This must be the last line
    }

    void TCPConn::HandleError() {
        //         int err = sockets::getSocketError(channel_->fd());
        //         LOG_ERROR << "TcpConnection::handleError [" << name_
        //             << "] - SO_ERROR = " << err << " " << strerror_tl(err);
    }

    void TCPConn::Send(const void* d, size_t dlen) {
        ::send(chan_->fd(), (const char*)d, dlen, 0); //TODO handle write error, handle the case that it is not the same IO thread
    }

    void TCPConn::OnAttachedToLoop() {
        loop_->AssertInLoopThread();
        chan_->AttachToLoop();
        chan_->EnableReadEvent();
    }
}