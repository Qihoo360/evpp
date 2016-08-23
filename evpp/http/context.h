#pragma once

#include "evpp/inner_pre.h"
#include "evpp/slice.h"

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

    // ��ȡԭʼURI���п��ܴ��в���, ����: /status.html?code=utf8
    const char* original_uri() const;

    // ��HTTP�����HEADER�в���ĳ��key��ֵ�����û���ҵ�����һ����ָ�롣
    const char* FindRequestHeader(const char* key);

    static std::string FindClientIP(const char* uri);



public:
    // ����������URI, ����: /status.html
    std::string uri;

    // Զ�̿ͻ���IP�������HTTP��������NGINXת�����������ǻ�����ʹ��URL�еġ�clientip������.
    // @see NGINX�������ο�����: proxy_pass http://127.0.0.1:8080/get/?clientip=$remote_addr;
    std::string remote_ip;

    // HTTP�����Body����
    Slice body;

    struct evhttp_request* req;
    EventLoop* dispatched_loop; // ����Ĺ����߳�
};

typedef std::shared_ptr<Context> ContextPtr;

typedef std::function<void(const std::string& response_data)> HTTPSendResponseCallback;

typedef std::function <
void(const ContextPtr& ctx,
     const HTTPSendResponseCallback& respcb) > HTTPRequestCallback;

typedef std::map<std::string/*The uri*/, HTTPRequestCallback> HTTPRequestCallbackMap;
}
}