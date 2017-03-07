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
    Context(struct evhttp_request* r);
    ~Context();

    bool Init();

    void AddResponseHeader(const std::string& key, const std::string& value);

    // The original URI, with original parameters, e.g. : /status.html?code=utf8
    const char* original_uri() const;

    // Finds the value belonging to a header.
    // 
    // @param key the name of the header to find
    // @returns a pointer to the value for the header or NULL if the header
    // could not be found.
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
    // The URI without any parameters : e.g. /status.html
    std::string uri_;

    // The remote client ip.
    // If the HTTP request is forworded by Nginx, 
    // we will prefer to use the value of 'clientip' parameter in URL
    // @see The reverse proxy Nginx configuration : proxy_pass http://127.0.0.1:8080/get/?clientip=$remote_addr;
    std::string remote_ip_;

    // The HTTP request body data
    Slice body_;

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
