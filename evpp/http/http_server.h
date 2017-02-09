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

// 这是一个可以独立运行的 HTTP Server
// 它会启动一个独立的线程用于端口监听、接收HTTP请求、分发HTTP请求、最后发送HTTP响应。
// 如果 thread_num 不为 0，它还会启动一个线程池，用于处理HTTP请求
class EVPP_EXPORT Server : public ThreadDispatchPolicy {
public:
    Server(uint32_t thread_num = 0);

    ~Server();

    bool Init(int listen_port);
    bool Init(const std::vector<int>& listen_ports);
    bool Init(const std::string& listen_ports/*like "80,8080,443"*/);
    bool AfterFork();
    bool Start();
    void Stop(bool wait_thread_exit = false);
    void Pause();
    void Continue();

    Service* service(int index = 0) const;
public:
    // uri 不能带有参数
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
        // 监听主线程，监听HTTP请求，接收HTTP请求数据和发送HTTP响应，将请求分发到工作线程
        std::shared_ptr<EventLoopThread> thread;

        // 每个线程运行一个HTTP Service用来监听HTTP请求
        std::shared_ptr<Service> hserver;
    };
    
    std::vector<ListenThread> listen_threads_;

    // 工作线程池，处理请求
    std::shared_ptr<EventLoopThreadPool> tpool_;

    HTTPRequestCallbackMap callbacks_;
    HTTPRequestCallback default_callback_;
};
}

}