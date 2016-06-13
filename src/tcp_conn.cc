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
                     , remote_addr_(raddr)
                     , type_(kIncoming)
                     , status_(kDisconnected)
                     , high_water_mark_(128 * 1024 * 1024)
    {
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
        assert(status_ == kDisconnected);
        assert(fd_ == chan_->fd());
        EVUTIL_CLOSESOCKET(fd_);
        fd_ = INVALID_SOCKET;
    }

    void TCPConn::Close() {
        loop_->RunInLoop(xstd::bind(&TCPConn::CloseInLoop, shared_from_this()));
    }

    void TCPConn::CloseInLoop() {
        loop_->AssertInLoopThread();
        HandleClose();
    }

    void TCPConn::Send(const std::string& d) {
        return Send(d.data(), d.size());
    }

    void TCPConn::Send(const Slice& message) {
        if (status_ == kConnected) {
            if (loop_->IsInLoopThread()) {
                SendInLoop(message);
            } else {
                loop_->RunInLoop(xstd::bind(&TCPConn::SendStringInLoop, shared_from_this(), message.ToString()));
            }
        }
    }

    void TCPConn::Send(const void* data, size_t len) {
        Send(Slice(static_cast<const char*>(data), len));
    }

    void TCPConn::Send(Buffer* buf) {
        if (status_ == kConnected) {
            if (loop_->IsInLoopThread()) {
                SendInLoop(buf->data(), buf->length());
                buf->NextAll();
            } else {
                loop_->RunInLoop(xstd::bind(&TCPConn::SendStringInLoop, this, buf->NextAllString()));
            }
        }
    }

    void TCPConn::HandleRead(Timestamp receiveTime) {
        loop_->AssertInLoopThread();
        int serrno = 0;
        ssize_t n = input_buffer_.ReadFromFD(chan_->fd(), &serrno);
        if (n > 0) {
            msg_fn_(shared_from_this(), &input_buffer_, receiveTime);
        } else if (n == 0) {
            if (type() == kOutgoing) {
                HandleClose();
            } else {
                //TODO
                HandleClose();
#if 0
                // incoming connection - we need to leave the request on the
                // connection so that we can reply to it.
                chan_->DisableReadEvent();
                LOG_DEBUG << "channel (fd=" << chan_->fd() << ") DisableReadEvent";
                loop_->RunAfter(
                    Duration(30.0),
                    makeWeakCallback(shared_from_this(), &TCPConn::Close));
#endif
            }

        } else {
            if (EVUTIL_ERR_RW_RETRIABLE(serrno)) {
                LOG_WARN << "TCPConn::HandleRead errno=" << serrno << " " << strerror(serrno);
            } else {
                HandleClose();
            }
        }
    }

    void TCPConn::HandleWrite() {
        loop_->AssertInLoopThread();
        assert(chan_->IsWritable());
        ssize_t n = ::send(fd_, output_buffer_.data(), output_buffer_.length(), 0);
        if (n > 0) {
            output_buffer_.Next(n);
            if (output_buffer_.length() == 0) {
                chan_->DisableWriteEvent();
                if (write_complete_fn_) {
                    loop_->QueueInLoop(xstd::bind(write_complete_fn_, shared_from_this()));
                }
            }
        } else {
            int serrno = errno;
            if (EVUTIL_ERR_RW_RETRIABLE(serrno)) {
                LOG_WARN << "TCPConn::HandleWrite errno=" << serrno << " " << strerror(serrno);
            } else {
                HandleClose();
            }
        }
    }

    void TCPConn::HandleClose() {
        assert(status_ == kConnected);
        status_ = kDisconnecting;
        loop_->AssertInLoopThread();
        chan_->DisableAllEvent();
        chan_->Close();

        TCPConnPtr conn(shared_from_this());
        if (conn_fn_) {
            conn_fn_(conn);
        }

        close_fn_(conn);
        status_ = kDisconnected;
    }

    void TCPConn::HandleError() {
    }

    void TCPConn::OnAttachedToLoop() {
        status_ = kConnected;
        loop_->AssertInLoopThread();
        chan_->AttachToLoop();
        chan_->EnableReadEvent();
        if (conn_fn_) {
            conn_fn_(shared_from_this());
        }
    }

    void TCPConn::SendInLoop(const Slice& message) {
        SendInLoop(message.data(), message.size());
    }

    void TCPConn::SendStringInLoop(const std::string& message) {
        SendInLoop(message.data(), message.size());
    }

    void TCPConn::SendInLoop(const void* data, size_t len) {
        loop_->AssertInLoopThread();
        ssize_t nwritten = 0;
        size_t remaining = len;
        bool write_error = false;
        if (status_ == kDisconnected) {
            LOG_WARN << "disconnected, give up writing";
            return;
        }

        // if no data in output queue, writing directly
        if (!chan_->IsWritable() && output_buffer_.length() == 0) {
            nwritten = ::send(chan_->fd(), static_cast<const char*>(data), len, 0);
            if (nwritten >= 0) {
                remaining = len - nwritten;
                if (remaining == 0 && write_complete_fn_) {
                    loop_->QueueInLoop(xstd::bind(write_complete_fn_, shared_from_this()));
                }
            } else {
                int serrno = errno;
                nwritten = 0;
                if (!EVUTIL_ERR_RW_RETRIABLE(serrno)) {
                    LOG_ERROR << "SendInLoop write failed errno=" << serrno << " " << strerror(serrno);
                    if (serrno == EPIPE || serrno == ECONNRESET) {
                        write_error = true;
                    }
                }
            }
        }

        assert(remaining <= len);
        if (!write_error && remaining > 0) {
            size_t old_len = output_buffer_.length();
            if (old_len + remaining >= high_water_mark_
                && old_len < high_water_mark_
                && high_water_mark_fn_) {
                loop_->QueueInLoop(xstd::bind(high_water_mark_fn_, shared_from_this(), old_len + remaining));
            }
            output_buffer_.Append(static_cast<const char*>(data)+nwritten, remaining);
            if (!chan_->IsWritable()) {
                chan_->EnableWriteEvent();
            }
        }
    }

    void TCPConn::SetHighWaterMarkCallback(const HighWaterMarkCallback& cb, size_t mark) {
        high_water_mark_fn_ = cb;
        high_water_mark_ = mark;
    }




}
