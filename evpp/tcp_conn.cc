#include "evpp/inner_pre.h"

#include "evpp/libevent_headers.h"

#include "evpp/tcp_conn.h"
#include "evpp/fd_channel.h"
#include "evpp/event_loop.h"

namespace evpp {
TCPConn::TCPConn(EventLoop* l,
                 const std::string& n,
                 int sockfd,
                 const std::string& laddr,
                 const std::string& raddr)
    : loop_(l)
    , fd_(sockfd)
    , name_(n)
    , local_addr_(laddr)
    , remote_addr_(raddr)
    , type_(kIncoming)
    , status_(kDisconnected)
    , high_water_mark_(128 * 1024 * 1024)
    , close_delay_(3.000001) {
    if (sockfd >= 0) {
        chan_.reset(new FdChannel(l, sockfd, false, false));
        chan_->SetReadCallback(std::bind(&TCPConn::HandleRead, this, std::placeholders::_1));
        chan_->SetWriteCallback(std::bind(&TCPConn::HandleWrite, this));
        chan_->SetCloseCallback(std::bind(&TCPConn::HandleClose, this));
    }

    LOG_DEBUG << "TCPConn::[" << name_ << "] this=" << this << " channel=" << chan_.get() << " fd=" << sockfd;
}

TCPConn::~TCPConn() {
    LOG_TRACE << "TCPConn::~TCPConn() name=" << name() << " this=" << this << " channel=" << chan_.get() << " fd=" << fd_ << " type=" << int(type()) << " status=" << StatusToString();
    assert(status_ == kDisconnected);

    if (fd_ >= 0) {
        assert(chan_);
        assert(fd_ == chan_->fd());
        assert(chan_->IsNoneEvent());
        EVUTIL_CLOSESOCKET(fd_);
        fd_ = INVALID_SOCKET;
    }
}

void TCPConn::Close() {
    auto c = shared_from_this();
    auto f = [c]() {
        assert(c->loop_->IsInLoopThread());
        c->HandleClose();
    };
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
        loop_->RunInLoop(std::bind(&TCPConn::SendStringInLoop, this, buf->NextAllString()));
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
        HandleClose();
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

void TCPConn::HandleRead(Timestamp recv_time) {
    assert(loop_->IsInLoopThread());
    int serrno = 0;
    ssize_t n = input_buffer_.ReadFromFD(chan_->fd(), &serrno);
    if (n > 0) {
        msg_fn_(shared_from_this(), &input_buffer_, recv_time);
    } else if (n == 0) {
        if (type() == kOutgoing) {
            // This is an outgoing connection, we own it and it's done. so close it
            HandleClose();
        } else {
            // Fix the half-closing problem��https://github.com/chenshuo/muduo/pull/117

            // This is an incoming connection, we need to preserve the connection for a while so that we can reply to it.
            // And we set a timer to close the connection eventually.
            chan_->DisableReadEvent();
            LOG_DEBUG << "channel (fd=" << chan_->fd() << ") DisableReadEvent";
            loop_->RunAfter(close_delay_, std::bind(&TCPConn::HandleClose, shared_from_this()));
        }
    } else {
        if (EVUTIL_ERR_RW_RETRIABLE(serrno)) {
            LOG_DEBUG << "TCPConn::HandleRead errno=" << serrno << " " << strerror(serrno);
        } else {
            LOG_DEBUG << "TCPConn::HandleRead errno=" << serrno << " " << strerror(serrno) << " We are closing this connection now.";
            HandleClose();
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
            LOG_WARN << "TCPConn::HandleWrite errno=" << serrno << " " << strerror(serrno);
        } else {
            HandleClose();
        }
    }
}

void TCPConn::HandleClose() {
    // ��ֹ��δ���
    if (status_ == kDisconnected) {
        return;
    }

    assert(status_ == kConnected);
    status_ = kDisconnecting;
    assert(loop_->IsInLoopThread());
    chan_->DisableAllEvent();
    chan_->Close();

    TCPConnPtr conn(shared_from_this());

    if (conn_fn_) {
        // Ӧ���� kDisconnecting ״̬�µ��øûص���
        // ���� TCPClient �ڶϿ�����ʱ��Ӧ�ò�����Լ������Ƿ���Ҫ�����Զ�������
        assert(status_ == kDisconnecting);
        conn_fn_(conn);
    }

    close_fn_(conn);
    status_ = kDisconnected;
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

std::string TCPConn::StatusToString() const {
    H_CASE_STRING_BIGIN(status_);
    H_CASE_STRING(kDisconnected);
    H_CASE_STRING(kConnecting);
    H_CASE_STRING(kConnected);
    H_CASE_STRING(kDisconnecting);
    H_CASE_STRING_END();
}
}
