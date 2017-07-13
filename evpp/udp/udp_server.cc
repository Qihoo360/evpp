#include "evpp/inner_pre.h"
#include "evpp/libevent.h"
#include "evpp/event_loop.h"
#include "evpp/event_loop_thread_pool.h"
#include "evpp/utility.h"

#include "udp_server.h"

namespace evpp {
namespace udp {

enum Status {
    kRunning = 1,
    kPaused = 2,
    kStopping = 3,
    kStopped = 4,
};

class Server::RecvThread {
public:
    RecvThread(Server* srv)
        : fd_(INVALID_SOCKET), server_(srv), port_(-1), status_(kStopped) {
    }

    ~RecvThread() {
        EVUTIL_CLOSESOCKET(fd_);
        fd_ = INVALID_SOCKET;
        if (this->thread_ && this->thread_->joinable()) {
            try {
                thread_->join();
            } catch (const std::system_error& e) {
                LOG_ERROR << "Caught a system_error:" << e.what();
            }
        }
    }

    bool Listen(int p) {
        this->port_ = p;
        this->fd_ = sock::CreateUDPServer(p);
        if (this->fd_ < 0) {
            LOG_ERROR << "listen error";
            return false;
        }
        sock::SetTimeout(this->fd_, 500);
        return true;
    }

    bool Run() {
        this->thread_.reset(new std::thread(std::bind(&Server::RecvingLoop, this->server_, this)));
        return true;
    }

    void Stop() {
        assert(IsRunning() || IsPaused());
        status_ = kStopping;
    }

    void Pause() {
        assert(IsRunning());
        status_ = kPaused;
    }

    void Continue() {
        assert(IsPaused());
        status_ = kRunning;
    }

    bool IsRunning() const {
        return status_ == kRunning;
    }

    bool IsStopped() const {
        return status_ == kStopped;
    }

    bool IsPaused() const {
        return status_ == kPaused;
    }

    void SetStatus(Status s) {
        status_ = s;
    }

    evpp_socket_t fd() const {
        return fd_;
    }

    int port() const {
        return port_;
    }

    Server* server() const {
        return server_;
    }
private:
    int fd_;
    Server* server_;
    int port_;
    std::shared_ptr<std::thread> thread_;
    Status status_;
};

Server::Server() : recv_buf_size_(1472) {}

Server::~Server() {
}

bool Server::Init(int port) {
    RecvThreadPtr t(new RecvThread(this));
    bool ret = t->Listen(port);
    assert(ret);
    recv_threads_.push_back(t);
    return ret;
}

bool Server::Init(const std::vector<int>& ports) {
    for (auto it : ports) {
        if (!Init(it)) {
            return false;
        }
    }
    return true;
}


bool Server::Init(const std::string& listen_ports/*like "53,5353,1053"*/) {
    std::vector<std::string> vec;
    StringSplit(listen_ports, ",", 0, vec);

    std::vector<int> v;
    for (auto& s : vec) {
        int i = std::atoi(s.c_str());
        if (i <= 0) {
            LOG_ERROR << "Cannot convert [" << s << "] to a integer. 'listen_ports' format wrong.";
            return false;
        }
        v.push_back(i);
    }

    return Init(v);
}

void Server::AfterFork() {
    // Nothing to do right now.
}

bool Server::Start() {
    if (!message_handler_) {
        LOG_ERROR << "MessageHandler DO NOT set!";
        return false;
    }

    for (auto& rt : recv_threads_) {
        if (!rt->Run()) {
            return false;
        }
    }

    while (!IsRunning()) {
        usleep(1);
    }
    return true;
}

void Server::Stop(bool wait_thread_exit) {
    for (auto& it : recv_threads_) {
        it->Stop();
    }

    if (wait_thread_exit) {
        while (!IsStopped()) {
            usleep(1);
        }
    }
}

void Server::Pause() {
    for (auto& it : recv_threads_) {
        it->Pause();
    }
}

void Server::Continue() {
    for (auto& it : recv_threads_) {
        it->Continue();
    }
}

bool Server::IsRunning() const {
    bool rc = true;
    for (auto& it : recv_threads_) {
        rc = rc && it->IsRunning();
    }

    return rc;
}

bool Server::IsStopped() const {
    bool rc = true;
    for (auto& it : recv_threads_) {
        rc = rc && it->IsStopped();
    }

    return rc;
}

void Server::RecvingLoop(RecvThread* thread) {
    LOG_INFO << "UDPServer is running at 0.0.0.0:" << thread->port();
    thread->SetStatus(kRunning);
    while (true) {
        if (thread->IsPaused()) {
            usleep(1);
            continue;
        }

        if (!thread->IsRunning()) {
            break;
        }

        // TODO use recvmmsg to improve performance

        MessagePtr recv_msg(new Message(thread->fd(), recv_buf_size_));
        socklen_t addr_len = sizeof(struct sockaddr);
        int readn = ::recvfrom(thread->fd(), (char*)recv_msg->WriteBegin(), recv_buf_size_, 0, recv_msg->mutable_remote_addr(), &addr_len);
        if (readn >= 0) {
            LOG_TRACE << "fd=" << thread->fd() << " port=" << thread->port()
                      << " recv len=" << readn << " from " << sock::ToIPPort(recv_msg->remote_addr());

            recv_msg->WriteBytes(readn);
            if (tpool_) {
                EventLoop* loop = nullptr;
                if (IsRoundRobin()) {
                    loop = tpool_->GetNextLoop();
                } else {
                    loop = tpool_->GetNextLoopWithHash(sock::sockaddr_in_cast(recv_msg->remote_addr())->sin_addr.s_addr);
                }
                loop->RunInLoop(std::bind(this->message_handler_, loop, recv_msg));
            } else {
                this->message_handler_(nullptr, recv_msg);
            }
        } else {
            int eno = errno;
            if (EVUTIL_ERR_RW_RETRIABLE(eno)) {
                continue;
            }

            LOG_ERROR << "errno=" << eno << " " << strerror(eno);
        }
    }

    LOG_INFO << "fd=" << thread->fd() << " port=" << thread->port() << " UDP server existed.";
    thread->SetStatus(kStopped);
}

}
}




/*
Benchmark data£ºIntel(R) Xeon(R) CPU E5-2630 0 @ 2.30GHz 24 core

The recvfrom thread is the bottleneck, other 23 working threads' load is very very low.

If we need to improve the performance, there two ways to achieve it:
1. Using Linux kernel 3.9+ SO_REUSEPORT
2. Using RAW SOCKET
3. Using recvmmsg/sendmmsg which can achieve 40w QPS on single thread

udp message length QPS£º
0.1k    9w+
1k      9w+

17:20:19       idgm/s    odgm/s  noport/s idgmerr/s
17:20:20     95572.00  95571.00      0.00      0.00
17:20:21     93522.00  93522.00      0.00      0.00
17:20:22     91669.00  91664.00      0.00      0.00
17:20:23     97165.00  97171.00      0.00      0.00
17:20:24     91225.00  91224.00      0.00      0.00
17:20:25     89659.00  89659.00      0.00      0.00
17:20:26     93199.00  93198.00      0.00      0.00
17:20:27     90758.00  90758.00      0.00      0.00
17:20:28     86891.00  86891.00      0.00      1.00
17:20:29     90346.00  90347.00      0.00      0.00
17:20:30     90675.00  90674.00      0.00      0.00
17:20:31     96589.00  96590.00      0.00      0.00
17:20:32     93739.00  93739.00      0.00      0.00
17:20:33     91374.00  91375.00      0.00      0.00
17:20:34     97162.00  97160.00      0.00      0.00
17:20:35     94281.00  94281.00      0.00      0.00
17:20:36     93101.00  93102.00      0.00      0.00
17:20:37     94741.00  94741.00      0.00      0.00
17:20:38     96064.00  96064.00      0.00      0.00
17:20:39     92436.00  92435.00      0.00      0.00
17:20:40     92432.00  92434.00      0.00      0.00
17:20:41     88199.00  88198.00      0.00      0.00
17:20:42     99852.00  99852.00      0.00      0.00
17:20:43     97449.00  97449.00      0.00      0.00
17:20:44     99380.00  99380.00      0.00      0.00
17:20:45     95372.00  95372.00      0.00      0.00
17:20:46     98629.00  98629.00      0.00      0.00
17:20:47     98187.00  98187.00      0.00      0.00
17:20:48     97442.00  97441.00      0.00      0.00
17:20:49     97482.00  97483.00      0.00      0.00
17:20:50     99234.00  99233.00      0.00      0.00
17:20:51     97863.00  97865.00      0.00      0.00
17:20:52     94651.00  94650.00      0.00      0.00
17:20:53     95824.00  95824.00      0.00      0.00
17:20:54     93032.00  93032.00      0.00      0.00
17:20:55     99538.00  99538.00      0.00      0.00
17:20:56     94344.00  94344.00      0.00      0.00
17:20:57    101252.00 101252.00      0.00      0.00
17:20:58     84523.00  84523.00      0.00      0.00
17:20:59     72918.00  72918.00      0.00      0.00
17:21:00     82758.00  82758.00      0.00      0.00
17:21:01     85132.00  85132.00      0.00      0.00
17:21:02     89151.00  89152.00      0.00      0.00
17:21:03     81456.00  81455.00      0.00      0.00
17:21:04     93089.00  93089.00      0.00      0.00
17:21:05     84559.00  84558.00      0.00      0.00
17:21:06     96240.00  96240.00      0.00      0.00
17:21:07     97439.00  97440.00      0.00      0.00
17:21:08     90971.00  90972.00      0.00      0.00
17:21:09     92930.00  92928.00      0.00      0.00
17:21:10     94668.00  94669.00      0.00      0.00
17:21:11     91937.00  91938.00      0.00      0.00
17:21:12     93480.00  93478.00      0.00      0.00
17:21:13     95384.00  95385.00      0.00      0.00
17:21:14     96525.00  96525.00      0.00      0.00
17:21:15    103088.00 103088.00      0.00      0.00
17:21:16     97340.00  97340.00      0.00      0.00
17:21:17     95124.00  95124.00      0.00      0.00
17:21:18     90760.00  90758.00      0.00      0.00
17:21:19     93717.00  93719.00      0.00      0.00
17:21:20     95226.00  95226.00      0.00      0.00
17:21:21     98472.00  98472.00      0.00      0.00
17:21:22     95193.00  95192.00      0.00      0.00
17:21:23     95296.00  95297.00      0.00      0.00
17:21:24     95517.00  95516.00      0.00      0.00
17:21:25     96691.00  96692.00      0.00      0.00
17:21:26     96200.00  96198.00      0.00      0.00
17:21:27     97431.00  97432.00      0.00      0.00

*/
