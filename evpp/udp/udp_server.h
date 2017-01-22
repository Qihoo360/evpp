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

    bool Start(int port);
    bool Start(const std::string& listen_ports);
    bool Start(const std::vector<int>& ports);
    void Stop(bool wait_thread_exit);

    void Pause();
    void Continue();

    //! \brief these functions to support fork for multiprocess program
    //! \brief call Init->fork process-> call StartWithPreInited
    bool Init(int port);
    bool Init(const std::vector<int>& ports);
    bool StartWithPreInited();

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

    // 工作线程池，用来处理UDP请求报文
    std::shared_ptr<EventLoopThreadPool> tpool_;

    size_t recv_buf_size_; // 接收udp包时开辟的缓冲区大小。最小值为1472，最大值为65535。默认值为1472。
private:
    void RecvingLoop(RecvThread* th);
};

}
}
