#pragma once

#include "evpp/inner_pre.h"
#include "evpp/thread_dispatch_policy.h"

#include "udp_message.h"

#include <thread>

namespace evpp {

class EventLoopThreadPool;
class EventLoop;

namespace udp {

class EVPP_EXPORT Server : public ThreadDispatchPolicy {
public:
    typedef std::function<void(EventLoop*, MessagePtr& msg)> MessageHandler;
public:
    Server();
    ~Server();

    bool Init(int port);
    bool Init(const std::vector<int>& ports);
    bool Init(const std::string& listen_ports/*like "53,5353,1053"*/);
    bool Start();
    void Stop(bool wait_thread_exit);

    void Pause();
    void Continue();

    // @brief Reinitialize some data fields after a fork
    void AfterFork();

    bool IsRunning() const;
    bool IsStopped() const;

    void SetMessageHandler(MessageHandler handler) {
        message_handler_ = handler;
    }

    void SetEventLoopThreadPool(const std::shared_ptr<EventLoopThreadPool>& pool) {
        tpool_ = pool;
    }

    void set_recv_buf_size(size_t v) {
        recv_buf_size_ = v;
    }

private:
    class RecvThread;
    typedef std::shared_ptr<RecvThread> RecvThreadPtr;
    std::vector<RecvThreadPtr> recv_threads_;

    MessageHandler   message_handler_;

    // The worker thread pool, used to process UDP package
    // This data field is not owned by UDPServer,
    // it is set by outer application layer.
    std::shared_ptr<EventLoopThreadPool> tpool_;

    // The buffer size used to receive an UDP package.
    // The minimum size is 1472, maximum size is 65535. Default : 1472
    // We can increase this size to receive a larger UDP package
    size_t recv_buf_size_;
private:
    void RecvingLoop(RecvThread* th);
};

}
}
