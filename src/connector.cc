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
        if (!IsConnected()) {
            // A connected tcp-connection's sockfd has been transfered to TCPConn.
            // But the sockfd of unconnected tcp-connections need to be closed by myself.
            LOG_TRACE << "Connector::~Connector close(" << chan_->fd() << ")";
            EVUTIL_CLOSESOCKET(chan_->fd());
        }
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
            if (!EVUTIL_ERR_CONNECT_RETRIABLE(serrno)) {
                HandleError();
                return;
            }
        }

        status_ = kConnecting;

        timer_.reset(new TimerEventWatcher(loop_, std::bind(&Connector::OnConnectTimeout, this), timeout_));
        timer_->Init();
        timer_->AsyncWait();

        chan_.reset(new FdChannel(loop_, fd, false, true));
        chan_->SetWriteCallback(std::bind(&Connector::HandleWrite, this));
        chan_->SetErrorCallback(std::bind(&Connector::HandleError, this));
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
            EVUTIL_SET_SOCKET_ERROR(err);
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
        int serrno = errno;
        LOG_INFO << "errno=" << serrno << " " << strerror(serrno);
        status_ = kDisconnected;
        chan_->Close();
        timer_->Cancel();
        if (EVUTIL_ERR_CONNECT_REFUSED(serrno)) {
            conn_fn_(-1, "");
        } else {
            loop_->RunAfter(1000, std::bind(&Connector::Start, this));//TODO Add retry times.
        }
    }

    void Connector::OnConnectTimeout() {
        assert(status_ == kConnecting);
        EVUTIL_SET_SOCKET_ERROR(ETIMEDOUT);
        HandleError();
    }

}