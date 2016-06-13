#include "evpp/inner_pre.h"

#include "evpp/connector.h"
#include "evpp/event_loop.h"
#include "evpp/fd_channel.h"
#include "evpp/sockets.h"
#include "evpp/libevent_headers.h"

namespace evpp {
    Connector::Connector(EventLoop* l, const std::string& raddr, Duration timeout)
        : status_(kDisconnected), loop_(l), remote_addr_(raddr), timeout_(timeout) {
        LOG_INFO << "Connector::Connector this=" << this << " raddr=" << raddr;
        raddr_ = ParseFromIPPort(remote_addr_.data());
    }

    Connector::~Connector() {
        loop_->AssertInLoopThread();
        chan_->Close();
        chan_.reset();
    }

    void Connector::Start() {
        LOG_INFO << "Try to connect " << remote_addr_;
        loop_->AssertInLoopThread();
        int fd = CreateNonblockingSocket();
        assert(fd >= 0);
        int rc = ::connect(fd, sockaddr_cast(&raddr_), sizeof(raddr_));
        if (rc != 0) {
            int serrno = errno;
            if (EVUTIL_ERR_CONNECT_RETRIABLE(serrno)) {
                // do nothing here
            } else if (EVUTIL_ERR_CONNECT_REFUSED(serrno)) {
                return;
            } else {
                HandleError();
                return;
            }
        }

        status_ = kConnecting;

        timer_.reset(new TimerEventWatcher(loop_->event_base(), xstd::bind(&Connector::OnConnectTimeout, this), timeout_));
        timer_->Init();
        timer_->AsyncWait();

        chan_.reset(new FdChannel(loop_, fd, false, true));
        chan_->SetWriteCallback(xstd::bind(&Connector::HandleWrite, this));
        chan_->SetErrorCallback(xstd::bind(&Connector::HandleError, this));
        chan_->AttachToLoop();
    }

    void Connector::HandleWrite() {
        if (status_ == kDisconnected) {
            LOG_INFO << "fd=" << chan_->fd() << " remote_addr=" << remote_addr_ << " receive write event when socket is closed";
            return;
        }

        assert(status_ == kConnecting);
        int err = 0;
        socklen_t len = sizeof(len);
        if (getsockopt(chan_->fd(), SOL_SOCKET, SO_ERROR, (char*)&err, (socklen_t *)&len) != 0) {
            err = errno;
            LOG_ERROR << "getsockopt failed err=" << err << " " << strerror(err);
        }

        if (err != 0) {
            HandleError();
            return;
        }

        chan_->DisableAllEvent();

        struct sockaddr_in addr = GetLocalAddr(chan_->fd());
        std::string laddr = ToIPPort(sockaddr_storage_cast(&addr));
        conn_fn_(chan_->fd(), laddr);
        timer_->Cancel();
        status_ = kConnected;
    }

    void Connector::HandleError() {
        LOG_INFO << "errno=" << errno << " " << strerror(errno);
        chan_->Close();
        timer_->Cancel();
        loop_->RunAfter(1000, xstd::bind(&Connector::Start, this));
    }

    void Connector::OnConnectTimeout() {
        assert(status_ == kConnecting);
        EVUTIL_SET_SOCKET_ERROR(ETIMEDOUT);
        HandleError();
    }

}