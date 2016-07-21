#pragma once

#include "evpp/inner_pre.h"
#include "udp_message.h"

#include <thread>

namespace evpp {
class EventLoopThreadPool;
namespace udp {
class EVPP_EXPORT Server {
public:
    typedef std::function<void(const MessagePtr& msg)> MessageHandler;
    enum ThreadDispatchPolicy {
        kRoundRobin,
        kIPAddressHashing,
    };
public:
    Server();
    ~Server();

    bool Start(int port);
    bool Start(std::vector<int> ports);
    void Stop(bool wait_thread_exit);

    void Pause();
    void Continue();

    bool IsRunning() const;
    bool IsStopped() const;

    void SetMessageHandler(MessageHandler handler) {
        message_handler_ = handler;
    }

    void SetEventLoopThreadPool(std::shared_ptr<EventLoopThreadPool>& pool) {
        tpool_ = pool;
    }

    void SetThreadDispatchPolicy(ThreadDispatchPolicy v) {
        threads_dispatch_policy_ = v;
    }
private:
    class RecvThread;
    typedef std::shared_ptr<RecvThread> RecvThreadPtr;
    std::vector<RecvThreadPtr> recv_threads_;

    MessageHandler   message_handler_;

    // 工作线程池，处理请求
    std::shared_ptr<EventLoopThreadPool> tpool_;
    ThreadDispatchPolicy threads_dispatch_policy_;
private:
    void RecvingLoop(RecvThread* th);
};

}
}
