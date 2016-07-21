#pragma once

#include "service.h"
#include "evpp/thread_dispatch_policy.h"

namespace evpp {
class EventLoopThreadPool;
class PipeEventWatcher;
class EventLoopThread;

namespace http {
class Service;

// 这是一个可以独立运行的 HTTP Server
// 它会启动一个独立的线程用于端口监听、接收HTTP请求、分发HTTP请求、最后发送HTTP响应。
// 如果 thread_num 不为 0，它还会启动一个线程池，用于处理HTTP请求
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

    // 监听主线程，监听http请求，接收HTTP请求数据和发送HTTP响应，将请求分发到工作线程
    std::shared_ptr<EventLoopThread> listen_thread_;

    // 工作线程池，处理请求
    std::shared_ptr<EventLoopThreadPool> tpool_;
};
}

}