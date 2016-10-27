#pragma once

#include "evpp/inner_pre.h"
#include "context.h"

struct evhttp;
struct evhttp_bound_socket;
namespace evpp {
class EventLoop;
class PipeEventWatcher;
namespace http {

// 一个可以嵌入到现有的EventLoop事件循环中的HTTP服务
// Service 不能独立运行，必须附着在某个EventLoop中
class EVPP_EXPORT Service {
public:
    Service(EventLoop* loop);
    ~Service();

    bool Listen(int port);
    void Stop();
    void Pause();
    void Continue();

    // uri 不能带有参数
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