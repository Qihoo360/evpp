#pragma once

#include "evpp/inner_pre.h"
#include "evpp/slice.h"
#include "evpp/timestamp.h"

#include <map>

struct evhttp_request;

namespace evpp {
class EventLoop;
}

namespace evpp {
namespace http {

class Service;

struct EVPP_EXPORT Context {
public:
    Context(struct evhttp_request* r, EventLoop* l);
    ~Context();

    bool Init(Service* hsrv);

    void AddResponseHeader(const std::string& key, const std::string& value);

    // 获取原始URI，有可能带有参数, 例如: /status.html?code=utf8
    const char* original_uri() const;

    // 在HTTP请求的HEADER中查找某个key的值。如果没有找到返回一个空指针。
    const char* FindRequestHeader(const char* key);

    const  std::string& uri() const {
        return uri_;
    }

    const std::string& remote_ip() const {
        return remote_ip_;
    }

    const Slice& body() const {
        return body_;
    }

    struct evhttp_request* req() const {
        return req_;
    }

public:
    static std::string FindClientIP(const char* uri);

private:
    // 不带参数的URI, 例如: /status.html
    std::string uri_;

    // 远程客户端IP。如果该HTTP请求是由NGINX转发而来，我们会优先使用URL中的‘clientip’参数.
    // @see NGINX反向代理参考配置: proxy_pass http://127.0.0.1:8080/get/?clientip=$remote_addr;
    std::string remote_ip_;

    // HTTP请求的Body数据
    Slice body_;

    // The request received timestamp
    //Timestamp recved_time_;
    //Timestamp dispached_time_;
    //Timestamp process_begin_time_;
    //Timestamp process_end_time_;
    //Timestamp send_response_time_;

    struct evhttp_request* req_;
};

typedef std::shared_ptr<Context> ContextPtr;

typedef std::function<void(const std::string& response_data)> HTTPSendResponseCallback;

typedef std::function <
    void(EventLoop* loop,
         const ContextPtr& ctx,
         const HTTPSendResponseCallback& respcb) > HTTPRequestCallback;

typedef std::map<std::string/*The uri*/, HTTPRequestCallback> HTTPRequestCallbackMap;
}
}