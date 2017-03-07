#pragma once

#include "evpp/inner_pre.h"
#include "context.h"

struct evhttp;
struct evhttp_bound_socket;
namespace evpp {
class EventLoop;
class PipeEventWatcher;
namespace http {

// A service does not run itself, it must be attached into a EventLoop
// So we can embed this Service to the existing EventLoop
class EVPP_EXPORT Service {
public:
    Service(EventLoop* loop);
    ~Service();

    bool Listen(int port);
    void Stop();
    void Pause();
    void Continue();

    // @Note The URI must not hold any parameters
    // @uri The URI of the request
    void RegisterHandler(const std::string& uri, HTTPRequestCallback callback);

    void RegisterDefaultHandler(HTTPRequestCallback callback);

    EventLoop* event_loop() const {
        return listen_loop_;
    }
private:
    static void GenericCallback(struct evhttp_request* req, void* arg);
    void HandleRequest(struct evhttp_request* req);
    void DefaultHandleRequest(const ContextPtr& ctx);
    void SendReply(struct evhttp_request* req, const std::string& response);
private:
    struct evhttp* evhttp_;
    struct evhttp_bound_socket* evhttp_bound_socket_;
    EventLoop* listen_loop_;
    HTTPRequestCallbackMap callbacks_;
    HTTPRequestCallback default_callback_;
};
}

}