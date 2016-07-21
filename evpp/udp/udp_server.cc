#include "evpp/inner_pre.h"
#include "udp_server.h"
#include "evpp/libevent_headers.h"

#include <thread>

namespace evpp {
    class UdpServer::Impl {
    public:
        Impl();
        ~Impl();

        //! Start the server.
        //! \remark Start the service and listening in the given port
        //!		This call will start several receiving thread at every net interface
        //! \return false if failed to start server.
        bool Start(std::vector<int> ports);
        bool Start(int port);

        //! Stop the server
        void Stop();

        bool IsRunning() const;
        bool IsStopped() const;

        void SetMessageHandler(MessageHandler handler) {
            message_handler_ = handler;
        }

    public:
        void set_name(const std::string& n);

    private:
        struct RecvThread {
            int		sockfd;
            Impl*		udp_server;
            std::string	ip;
            int			port;
            std::shared_ptr<std::thread> thread;
            Status status;
        };
        typedef std::shared_ptr<RecvThread> RecvThreadPtr;

        typedef std::vector<RecvThreadPtr> RecvThreadVector;
        RecvThreadVector recv_threads_;
        MessageHandler   message_handler_;

        //For unit test
    private:
        std::string      name_;
    private:
        void RecvingLoop(RecvThread* th);
    };

    UdpServer::Impl::Impl() {}

    UdpServer::Impl::~Impl() {}

    bool UdpServer::Impl::Start(std::vector<int> ports) {
        std::vector<int>::const_iterator pit = ports.begin();
        std::vector<int>::const_iterator pite = ports.end();
        for (; pit != pite; ++pit) {
            if (!Start(*pit)) {
                return false;
            }
        }
        return true;
    }

    bool UdpServer::Impl::Start(int port) {
        if (!message_handler_) {
            LOG_ERROR << "MessageHandler DO NOT set!";
            return false;
        }
        RecvThreadPtr th(new RecvThread);
        th->status = UdpServer::kRunning;
        th->port = port;
        th->sockfd = CreateUDPServer(port);
        th->udp_server = this;
        th->thread.reset(new std::thread(std::bind(&UdpServer::Impl::RecvingLoop, this, th.get())));
        SetTimeout(th->sockfd, 500);
        LOG_TRACE << "start udp server at 0.0.0.0:" << port;
        recv_threads_.push_back(th);
        return true;
    }

    void UdpServer::Impl::Stop() {
        RecvThreadVector::iterator it(recv_threads_.begin());
        RecvThreadVector::iterator ite(recv_threads_.end());
        for (; it != ite; it++) {
            (*it)->status = UdpServer::kStopping;
        }
    }

    bool UdpServer::Impl::IsRunning() const {
        bool rc = true;
        RecvThreadVector::const_iterator it(recv_threads_.begin());
        RecvThreadVector::const_iterator ite(recv_threads_.end());
        for (; it != ite; it++) {
            rc = rc && (*it)->status == UdpServer::kRunning;
        }

        return rc;
    }

    bool UdpServer::Impl::IsStopped() const {
        bool rc = true;
        RecvThreadVector::const_iterator it(recv_threads_.begin());
        RecvThreadVector::const_iterator ite(recv_threads_.end());
        for (; it != ite; it++) {
            rc = rc && (*it)->status == UdpServer::kStopped;
        }

        return rc;
    }

    inline bool IsEAgain(int eno) {
#ifdef H_OS_WINDOWS 
        return (eno == WSAEWOULDBLOCK);
#else
        return (eno == EAGAIN);
#endif
    }

    void UdpServer::Impl::RecvingLoop(RecvThread* th) {
        while (th->status == UdpServer::kRunning) {
            size_t nBufSize = 1472; // TODO The UDP max payload size
            UdpMessagePtr recv_msg(new UdpMessage(th->sockfd, nBufSize));
            socklen_t m_nAddrLen = sizeof(struct sockaddr);
            int readn = ::recvfrom(th->sockfd, (char*)recv_msg->data(), nBufSize, 0, recv_msg->mutable_remote_addr(), &m_nAddrLen);
            LOG_TRACE << "fd=" << th->sockfd << " port=" << th->port
                << " recv len=" << readn << " from " << ToIPPort(sockaddr_storage_cast(recv_msg->remote_addr()));
            if (readn >= 0) {
                recv_msg->WriteBytes(readn);
                th->udp_server->message_handler_(recv_msg);
            } else {
                int eno = errno;
                if (IsEAgain(eno)) {
                    continue;
                }

                LOG_ERROR << "errno=" << eno << " " << strerror(eno);
            }
        }
        th->status = UdpServer::kStopped;
        LOG_INFO << "fd=" << th->sockfd << " port=" << th->port << " UDP server(" << name_ << ") existed.";
        EVUTIL_CLOSESOCKET(th->sockfd);
    }

    void UdpServer::Impl::set_name(const std::string& n) {
        name_ = n;
    }



    //////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////


    void UdpServer::set_name(const std::string& n) {
        impl_->set_name(n);
    }
    void UdpServer::SetMessageHandler(MessageHandler handler) {
        impl_->SetMessageHandler(handler);
    }

    bool UdpServer::Start(int port) {
        return impl_->Start(port);
    }

    bool UdpServer::Start(std::vector<int> ports) {
        return impl_->Start(ports);
    }

    void UdpServer::Stop() {
        Stop(true);
    }

    void UdpServer::Stop(bool wait_thread_exit) {
        impl_->Stop();
        if (wait_thread_exit) {
            while (!impl_->IsStopped()) {
                usleep(1);
            }
        }
    }

    bool UdpServer::IsRunning() const {
        return impl_->IsRunning();
    }

    bool UdpServer::IsStopped() const {
        return impl_->IsStopped();
    }

    UdpServer::UdpServer() {
        impl_.reset(new UdpServer::Impl);
    }

    UdpServer::~UdpServer() {
        impl_->Stop();
        impl_.reset();
    }

}




/*
性能测试数据：Intel(R) Xeon(R) CPU E5-2630 0 @ 2.30GHz 24核

性能瓶颈卡在recvfrom接收线程上，其他23个工作线程毫无压力！

udp message不同长度下的QPS：
0.1k 9w
1k   8w


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
