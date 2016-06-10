#include "evpp/inner_pre.h"

#include "evpp/connector.h"
#include "evpp/event_loop.h"
#include "evpp/fd_channel.h"
#include "evpp/sockets.h"
#include "evpp/libevent_headers.h"

namespace evpp {
    Connector::Connector(EventLoop* l, const std::string& raddr)
        : status_(kDisconnected), loop_(l), remote_addr_(raddr) {
        LOG_INFO << "Connector::Connector this=" << this << " raddr=" << raddr;
    }

    void Connector::Start() {
        loop_->AssertInLoopThread();
        int fd = CreateNonblockingSocket();
        assert(fd >= 0);
        const struct sockaddr_in addr = ParseFromIPPort(remote_addr_.data());
        int rc = ::connect(fd, sockaddr_cast(&addr), sizeof(addr));
        if (rc != 0) {
            int serrno = errno;
            if (serrno != EINPROGRESS && serrno != EWOULDBLOCK && serrno != EISCONN) {
                //TODO ERROR retry
                return;
            }
        }
        status_ = kConnecting;

        //TODO add a timeout timer
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
    }

    void Connector::HandleError() {
        EVUTIL_CLOSESOCKET(chan_->fd());
        loop_->RunAfter(1000, xstd::bind(&Connector::Start, this));
    }
}