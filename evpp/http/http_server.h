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

// This is a standalone running HTTP server. It will start a new thread
// for every listening port and we give this thread a name 'listening main thread'.
//
// The listening main thread has responsibility to listen a TCP port,
// receive HTTP request, dispatch the request and send the HTTP response.
//
// if the thread_num is not 0, it will also start new worker thread pool
// to help process HTTP request.
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
    // @Note The URI must not hold any parameters
    // @uri The URI of the request
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
        // The listening main thread
        std::shared_ptr<EventLoopThread> thread;

        // Every listening main thread runs a HTTP Service to listen, receive, dispatch, send response the HTTP request.
        std::shared_ptr<Service> hserver;
    };

    std::vector<ListenThread> listen_threads_;

    // The worker thread pool used to process HTTP request
    std::shared_ptr<EventLoopThreadPool> tpool_;

    HTTPRequestCallbackMap callbacks_;
    HTTPRequestCallback default_callback_;
};
}

}