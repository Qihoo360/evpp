#pragma once

#include "evpp/inner_pre.h"
#include "evpp/thread_dispatch_policy.h"

#include "udp_message.h"

#include <thread>

namespace evpp {
class EventLoopThreadPool;
namespace udp {
class EVPP_EXPORT Server : public ThreadDispatchPolicy {
public:
    typedef std::function<void(const MessagePtr& msg)> MessageHandler;

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

    void set_recv_buf_size(size_t v) {
        recv_buf_size_ = v;
    }

private:
    class RecvThread;
    typedef std::shared_ptr<RecvThread> RecvThreadPtr;
    std::vector<RecvThreadPtr> recv_threads_;

    MessageHandler   message_handler_;

    // �����̳߳أ���������
    std::shared_ptr<EventLoopThreadPool> tpool_;

    size_t recv_buf_size_; // ����udp��ʱ�Ļ�������С��Ĭ��ֵΪ1472����СֵΪ1472�����ֵΪ65535��
private:
    void RecvingLoop(RecvThread* th);
};

}
}
