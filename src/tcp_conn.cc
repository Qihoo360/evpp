#include "evpp/inner_pre.h"

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

    void TCPConn::HandleRead(base::Timestamp receiveTime) {
        loop_->AssertInLoopThread();
        int serrno = 0;
        ssize_t n = inputBuffer_.ReadFromFD(chan_->fd(), &serrno);
        if (n > 0) {
            messageCallback_(shared_from_this(), &inputBuffer_, receiveTime);
        } else if (n == 0) {
            HandleClose();
        } else {
            errno = serrno;
            LOG_ERROR << "TCPConn::HandleRead";
            HandleError();
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
        chan_->Close();
        chan_.reset();

        //         LOG_TRACE << "fd = " << channel_->fd() << " state = " << stateToString();
        //         assert(state_ == kConnected || state_ == kDisconnecting);
        //         // we don't close fd, leave it to dtor, so we can find leaks easily.
        //         setState(kDisconnected);
        //         channel_->disableAll();
        // 
        //         TcpConnectionPtr guardThis(shared_from_this());
        //         connectionCallback_(guardThis);
        //         // must be the last line
        //         closeCallback_(guardThis);
    }

    void TCPConn::HandleError() {
        //         int err = sockets::getSocketError(channel_->fd());
        //         LOG_ERROR << "TcpConnection::handleError [" << name_
        //             << "] - SO_ERROR = " << err << " " << strerror_tl(err);
    }

    void TCPConn::Send(const void* d, size_t dlen) {
        ::send(chan_->fd(), (const char*)d, dlen, 0); //TODO handler write error
    }

    void TCPConn::OnAttachedToLoop() {
        loop_->AssertInLoopThread();
        chan_->AttachToLoop();
        chan_->EnableReadEvent();
    }

}