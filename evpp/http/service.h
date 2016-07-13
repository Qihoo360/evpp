#pragma once

#include "evpp/inner_pre.h"
#include "context.h"

struct evhttp;

namespace evpp {
class EventLoop;
class PipeEventWatcher;
namespace http {
class EVPP_EXPORT Service {
public:
    Service(EventLoop* loop);
    ~Service();

    bool Listen(int port);

    void Stop();

    // uri ���ܴ��в���
    bool RegisterHandler(const std::string& uri, HTTPRequestCallback callback);
    bool RegisterDefaultHandler(HTTPRequestCallback callback);

private:
    static void GenericCallback(struct evhttp_request* req, void* arg);
    void HandleRequest(struct evhttp_request* req);
    void DefaultHandleRequest(const ContextPtr& ctx);
    void SendReply(struct evhttp_request* req, const std::string& response);
private:
    struct evhttp* evhttp_;
    EventLoop* listen_loop_;
    HTTPRequestCallbackMap callbacks_;
    HTTPRequestCallback default_callback_;
};
}

}