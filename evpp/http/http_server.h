#pragma once

#include "service.h"
#include "evpp/thread_dispatch_policy.h"

namespace evpp {
class EventLoop;
class EventLoopThreadPool;
class PipeEventWatcher;
class EventLoopThread;

namespace http {
class Service;

// ����һ�����Զ������е� HTTP Server
// ��������һ���������߳����ڶ˿ڼ���������HTTP���󡢷ַ�HTTP���������HTTP��Ӧ��
// ��� thread_num ��Ϊ 0������������һ���̳߳أ����ڴ���HTTP����
class EVPP_EXPORT Server : public ThreadDispatchPolicy {
public:
    Server(uint32_t thread_num = 0);

    ~Server();

    bool Start(int listen_port);
    bool Start(const std::vector<int>& listen_ports); // Ϊÿ�������˿ڷֱ�����һ���߳�
    void Stop(bool wait_thread_exit = false);
    void Pause();
    void Continue();

    //! \brief these functions to support fork for multiprocess program
    //! \brief call Init->fork process-> call AfterFork-> call StartWithPreInited
    bool Init(int listen_port);
    bool Init(const std::vector<int>& listen_ports);
    bool AfterFork();
    bool StartWithPreInited();


    Service* service(int index = 0) const;
public:
    // uri ���ܴ��в���
    void RegisterHandler(const std::string& uri,
                         HTTPRequestCallback callback);

    void RegisterDefaultHandler(HTTPRequestCallback callback);
public:
    bool IsRunning() const;
    bool IsStopped() const;

    std::shared_ptr<EventLoopThreadPool> pool() const {
        return tpool_;
    }
private:
    void Dispatch(EventLoop* listening_loop,
                  const ContextPtr& ctx,
                  const HTTPSendResponseCallback& response_callback,
                  const HTTPRequestCallback& user_callback);

    EventLoop* GetNextLoop(EventLoop* default_loop, const ContextPtr& ctx);

private:
    struct ListenThread {
        // �������̣߳�����HTTP���󣬽���HTTP�������ݺͷ���HTTP��Ӧ��������ַ��������߳�
        std::shared_ptr<EventLoopThread> thread;

        // ÿ���߳�����һ��HTTP Service��������HTTP����
        std::shared_ptr<Service> hserver;
    };
    
    std::vector<ListenThread> listen_threads_;

    // �����̳߳أ���������
    std::shared_ptr<EventLoopThreadPool> tpool_;

    HTTPRequestCallbackMap callbacks_;
    HTTPRequestCallback default_callback_;
};
}

}