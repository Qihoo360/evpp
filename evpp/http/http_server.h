#pragma once

#include <atomic>

#include "service.h"
#include "evpp/thread_dispatch_policy.h"
#include "evpp/server_status.h"

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
// If the thread_num is not 0, it will also start a working thread pool
// to help process HTTP requests.
// The typical usage is :
//      1. Create a Server object
//      2. Set the message callback and connection callback
//      3. Call Server::Init()
//      4. Call Server::Start()
//      5. Process HTTP request in callbacks
//      6. At last call Server::Stop() to stop the whole server
class EVPP_EXPORT Server : public ThreadDispatchPolicy, public ServerStatus {
public:
    Server(uint32_t thread_num = 0);

    ~Server();

    bool Init(int listen_port);
    bool Init(const std::vector<int>& listen_ports);
    bool Init(const std::string& listen_ports/*like "80,8080,443"*/);

    bool Start();

    void Stop();

    void Pause();
    void Continue();

    // @brief Reinitialize the event_base object after a fork
    void AfterFork();

public:
    // @Note The URI must not hold any parameters
    // @param uri - The URI of the request without any parameters
    void RegisterHandler(const std::string& uri,
                         HTTPRequestCallback callback);

    void RegisterDefaultHandler(HTTPRequestCallback callback);
public:

    std::shared_ptr<EventLoopThreadPool> pool() const {
        return tpool_;
    }

    // Get the service object hold by this http server.
    Service* service(int index = 0) const;
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
        std::shared_ptr<Service> hservice;
    };

    std::vector<ListenThread> listen_threads_;

    // The worker thread pool used to process HTTP request
    std::shared_ptr<EventLoopThreadPool> tpool_;

    HTTPRequestCallbackMap callbacks_;
    HTTPRequestCallback default_callback_;
};
}

}