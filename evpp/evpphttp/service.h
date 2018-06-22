#pragma once
#include "evpp/inner_pre.h"
#include "evpp/event_loop.h"
#include "evpp/tcp_server.h"
#include "evpp/evpphttp/http_request.h"
#include "evpp/evpphttp/http_response.h"
namespace evpp {
namespace evpphttp {
typedef std::function<void(const int response_code, const std::map<std::string, std::string>& response_field_value, const std::string& response_data)> HTTPSendResponseCallback;
typedef std::function <void(EventLoop* loop, HttpRequest& ctx, const HTTPSendResponseCallback& respcb)> HTTPRequestCallback;
class EVPP_EXPORT Service {
private:
    typedef std::map<std::string/*The uri*/, HTTPRequestCallback> HTTPRequestCallbackMap;
public:
    Service(const std::string& listen_addr, const std::string& name, uint32_t thread_num);
    ~Service();
    void Stop();
    bool Init(const ConnectionCallback& cb = [](const TCPConnPtr& conn) {
        conn->SetTCPNoDelay(true);
    });
    bool Start();
    void RegisterHandler(const std::string& uri, const HTTPRequestCallback& callback);
    inline bool IsStopped() const {
        return is_stopped_;
    }

    inline void set_default_cb(const HTTPRequestCallback& cb) {
        default_callback_ = cb;
    }

    void AfterFork();

private:
    int RequestHandler(const evpp::TCPConnPtr& conn, evpp::Buffer* buf, HttpRequest& hr);
    void OnMessage(const evpp::TCPConnPtr& conn, evpp::Buffer* buf);
private:
    std::string listen_addr_;
    std::string name_;
    const int thread_num_;
    EventLoop* listen_loop_{nullptr};
    TCPServer* tcp_srv_{nullptr};
    std::thread * listen_thr_{nullptr};
    HTTPRequestCallback default_callback_;
    HTTPRequestCallbackMap callbacks_;
    bool is_stopped_{false};
};
}
}
