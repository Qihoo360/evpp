#include "evpp/inner_pre.h"

#include "evpp/libevent.h"

#include "evpp/tcp_conn.h"
#include "evpp/fd_channel.h"
#include "evpp/event_loop.h"
#include "evpp/sockets.h"
#include "evpp/invoke_timer.h"

namespace evpp {
TCPConn::TCPConn(EventLoop* l,
                 const std::string& n,
                 evpp_socket_t sockfd,
                 const std::string& laddr,
                 const std::string& raddr,
                 uint64_t conn_id)
    : loop_(l)
    , fd_(sockfd)
    , id_(conn_id)
    , name_(n)
    , local_addr_(laddr)
    , remote_addr_(raddr)
    , type_(kIncoming)
    , status_(kDisconnected) {
    if (sockfd >= 0) {
        chan_.reset(new FdChannel(l, sockfd, false, false));
        chan_->SetReadCallback(std::bind(&TCPConn::HandleRead, this));
        chan_->SetWriteCallback(std::bind(&TCPConn::HandleWrite, this));
    }

    DLOG_TRACE << "TCPConn::[" << name_ << "] channel=" << chan_.get() << " fd=" << sockfd << " addr=" << AddrToString();
}

TCPConn::~TCPConn() {
    DLOG_TRACE << "name=" << name()
        << " channel=" << chan_.get()
        << " fd=" << fd_ << " type=" << int(type())
        << " status=" << StatusToString() << " addr=" << AddrToString();
    assert(status_ == kDisconnected);

    if (fd_ >= 0) {
        assert(chan_);
        assert(fd_ == chan_->fd());
        assert(chan_->IsNoneEvent());
        EVUTIL_CLOSESOCKET(fd_);
        fd_ = INVALID_SOCKET;
    }

    assert(!delay_close_timer_.get());
}

void TCPConn::Close() {
    DLOG_TRACE << "fd=" << fd_ << " status=" << StatusToString() << " addr=" << AddrToString();
    status_ = kDisconnecting;
    auto c = shared_from_this();
    auto f = [c]() {
        assert(c->loop_->IsInLoopThread());
        c->HandleClose();
    };

    // Use QueueInLoop to fix TCPClient::Close bug when the application delete TCPClient in callback
    loop_->QueueInLoop(f);
}

void TCPConn::Send(const std::string& d) {
    if (status_ != kConnected) {
        return;
    }

    if (loop_->IsInLoopThread()) {
        SendInLoop(d);
    } else {
        loop_->RunInLoop(std::bind(&TCPConn::SendStringInLoop, shared_from_this(), d));
    }
}

void TCPConn::Send(const Slice& message) {
    if (status_ != kConnected) {
        return;
    }

    if (loop_->IsInLoopThread()) {
        SendInLoop(message);
    } else {
        loop_->RunInLoop(std::bind(&TCPConn::SendStringInLoop, shared_from_this(), message.ToString()));
    }
}

void TCPConn::Send(const void* data, size_t len) {
    if (loop_->IsInLoopThread()) {
        SendInLoop(data, len);
        return;
    }
    Send(Slice(static_cast<const char*>(data), len));
}

void TCPConn::Send(Buffer* buf) {
    if (status_ != kConnected) {
        return;
    }

    if (loop_->IsInLoopThread()) {
        SendInLoop(buf->data(), buf->length());
        buf->Reset();
    } else {
        loop_->RunInLoop(std::bind(&TCPConn::SendStringInLoop, shared_from_this(), buf->NextAllString()));
    }
}

void TCPConn::SendInLoop(const Slice& message) {
    SendInLoop(message.data(), message.size());
}

void TCPConn::SendStringInLoop(const std::string& message) {
    SendInLoop(message.data(), message.size());
}

void TCPConn::SendInLoop(const void* data, size_t len) {
    assert(loop_->IsInLoopThread());

    if (status_ == kDisconnected) {
        LOG_WARN << "disconnected, give up writing";
        return;
    }

    ssize_t nwritten = 0;
    size_t remaining = len;
    bool write_error = false;

    // if no data in output queue, writing directly
    if (!chan_->IsWritable() && output_buffer_.length() == 0) {
        nwritten = ::send(chan_->fd(), static_cast<const char*>(data), len, MSG_NOSIGNAL);
        if (nwritten >= 0) {
            remaining = len - nwritten;
            if (remaining == 0 && write_complete_fn_) {
                loop_->QueueInLoop(std::bind(write_complete_fn_, shared_from_this()));
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

    if (write_error) {
        HandleError();
        return;
    }

    assert(!write_error);
    assert(remaining <= len);

    if (remaining > 0) {
        size_t old_len = output_buffer_.length();
        if (old_len + remaining >= high_water_mark_
                && old_len < high_water_mark_
                && high_water_mark_fn_) {
            loop_->QueueInLoop(std::bind(high_water_mark_fn_, shared_from_this(), old_len + remaining));
        }

        output_buffer_.Append(static_cast<const char*>(data) + nwritten, remaining);

        if (!chan_->IsWritable()) {
            chan_->EnableWriteEvent();
        }
    }
}

void TCPConn::HandleRead() {
    assert(loop_->IsInLoopThread());
    int serrno = 0;
    ssize_t n = input_buffer_.ReadFromFD(chan_->fd(), &serrno);
    if (n > 0) {
        msg_fn_(shared_from_this(), &input_buffer_);
    } else if (n == 0) {
        if (type() == kOutgoing) {
            // This is an outgoing connection, we own it and it's done. so close it
            DLOG_TRACE << "fd=" << fd_ << ". We read 0 bytes and close the socket.";
            status_ = kDisconnecting;
            HandleClose();
        } else {
            // Fix the half-closing problem : https://github.com/chenshuo/muduo/pull/117

            chan_->DisableReadEvent();
            if (close_delay_.IsZero()) {
                DLOG_TRACE << "channel (fd=" << chan_->fd() << ") DisableReadEvent. delay time " << close_delay_.Seconds() << "s. We close this connection immediately";
                DelayClose();
            } else {
                // This is an incoming connection, we need to preserve the
                // connection for a while so that we can reply to it.
                // And we set a timer to close the connection eventually.
                DLOG_TRACE << "channel (fd=" << chan_->fd() << ") DisableReadEvent. And set a timer to delay close this TCPConn, delay time " << close_delay_.Seconds() << "s";
                delay_close_timer_ = loop_->RunAfter(close_delay_, std::bind(&TCPConn::DelayClose, shared_from_this())); // TODO leave it to user layer close.
            }
        }
    } else {
        if (EVUTIL_ERR_RW_RETRIABLE(serrno)) {
            DLOG_TRACE << "errno=" << serrno << " " << strerror(serrno);
        } else {
            DLOG_TRACE << "errno=" << serrno << " " << strerror(serrno) << " We are closing this connection now.";
            HandleError();
        }
    }
}

void TCPConn::HandleWrite() {
    assert(loop_->IsInLoopThread());
    assert(!chan_->attached() || chan_->IsWritable());

    ssize_t n = ::send(fd_, output_buffer_.data(), output_buffer_.length(), MSG_NOSIGNAL);
    if (n > 0) {
        output_buffer_.Next(n);

        if (output_buffer_.length() == 0) {
            chan_->DisableWriteEvent();

            if (write_complete_fn_) {
                loop_->QueueInLoop(std::bind(write_complete_fn_, shared_from_this()));
            }
        }
    } else {
        int serrno = errno;

        if (EVUTIL_ERR_RW_RETRIABLE(serrno)) {
            LOG_WARN << "this=" << this << " TCPConn::HandleWrite errno=" << serrno << " " << strerror(serrno);
        } else {
            HandleError();
        }
    }
}

void TCPConn::DelayClose() {
    assert(loop_->IsInLoopThread());
    DLOG_TRACE << "addr=" << AddrToString() << " fd=" << fd_ << " status_=" << StatusToString();
    status_ = kDisconnecting;
    delay_close_timer_.reset();
    HandleClose();
}

void TCPConn::HandleClose() {
    DLOG_TRACE << "addr=" << AddrToString() << " fd=" << fd_ << " status_=" << StatusToString();

    // Avoid multi calling
    if (status_ == kDisconnected) {
        return;
    }

    // We call HandleClose() from TCPConn's method, the status_ is kConnected
    // But we call HandleClose() from out of TCPConn's method, the status_ is kDisconnecting
    assert(status_ == kDisconnecting);

    status_ = kDisconnecting;
    assert(loop_->IsInLoopThread());
    chan_->DisableAllEvent();
    chan_->Close();

    TCPConnPtr conn(shared_from_this());

    if (delay_close_timer_) {
        DLOG_TRACE << "loop=" << loop_ << " Cancel the delay closing timer.";
        delay_close_timer_->Cancel();
        delay_close_timer_.reset();
    }

    if (conn_fn_) {
        // This callback must be invoked at status kDisconnecting
        // e.g. when the TCPClient disconnects with remote server,
        // the user layer can decide whether to do the reconnection.
        assert(status_ == kDisconnecting);
        conn_fn_(conn);
    }

    if (close_fn_) {
        close_fn_(conn);
    }
    DLOG_TRACE << "addr=" << AddrToString() << " fd=" << fd_ << " status_=" << StatusToString() << " use_count=" << conn.use_count();
    status_ = kDisconnected;
}

void TCPConn::HandleError() {
    DLOG_TRACE << "fd=" << fd_ << " status=" << StatusToString();
    status_ = kDisconnecting;
    HandleClose();
}

void TCPConn::OnAttachedToLoop() {
    assert(loop_->IsInLoopThread());
    status_ = kConnected;
    chan_->EnableReadEvent();

    if (conn_fn_) {
        conn_fn_(shared_from_this());
    }
}

void TCPConn::SetHighWaterMarkCallback(const HighWaterMarkCallback& cb, size_t mark) {
    high_water_mark_fn_ = cb;
    high_water_mark_ = mark;
}

void TCPConn::SetTCPNoDelay(bool on) {
    sock::SetTCPNoDelay(fd_, on);
}

std::string TCPConn::StatusToString() const {
    H_CASE_STRING_BIGIN(status_);
    H_CASE_STRING(kDisconnected);
    H_CASE_STRING(kConnecting);
    H_CASE_STRING(kConnected);
    H_CASE_STRING(kDisconnecting);
    H_CASE_STRING_END();
}
}
