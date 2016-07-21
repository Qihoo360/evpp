#pragma once

#include "service.h"
#include "evpp/thread_dispatch_policy.h"

namespace evpp {
class EventLoopThreadPool;
class PipeEventWatcher;
class EventLoopThread;

namespace http {
class Service;

// ����һ�����Զ������е� HTTP Server
// ��������һ���������߳����ڶ˿ڼ���������HTTP���󡢷ַ�HTTP���������HTTP��Ӧ��
// ��� thread_num ��Ϊ 0������������һ���̳߳أ����ڴ���HTTP����
class EVPP_EXPORT HTTPServer : public ThreadDispatchPolicy {
public:
    HTTPServer(uint32_t thread_num = 0);

    ~HTTPServer();

    bool Start(int listen_port);
    void Stop(bool wait_thread_exit = false);
    void Pause();
    void Continue();

    Service* service() const;
public:
    bool RegisterHandler(const std::string& uri,
                         HTTPRequestCallback callback);

    bool RegisterDefaultHandler(HTTPRequestCallback callback);
public:
    bool IsRunning() const;
    bool IsStopped() const;

    std::shared_ptr<EventLoopThreadPool> pool() const {
        return tpool_;
    }

private:
    void Dispatch(const ContextPtr& ctx,
                  const HTTPSendResponseCallback& response_callback,
                  const HTTPRequestCallback& user_callback);

private:
    std::shared_ptr<Service>   http_;

    // �������̣߳�����http���󣬽���HTTP�������ݺͷ���HTTP��Ӧ��������ַ��������߳�
    std::shared_ptr<EventLoopThread> listen_thread_;

    // �����̳߳أ���������
    std::shared_ptr<EventLoopThreadPool> tpool_;
};
}

}