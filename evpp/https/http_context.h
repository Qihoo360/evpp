#pragma once



#include "evpp/inner_pre.h"
#include "evpp/slice.h"

#include <map>
#include <set>

struct event_base;
struct evhttp;
struct evhttp_request;
struct evbuffer;

namespace evpp {
    namespace https {
        class HTTPService;
        typedef std::map<std::string, std::string> HTTPParameterMap;
        struct EVPP_EXPORT HTTPContext {
            HTTPContext(struct evhttp_request* r);
            ~HTTPContext();

            bool Init(HTTPService* hsrv);

            const char* original_uri() const; // Get the original URI with parameters

            const std::string& GetRequestHeader(const std::string& key, bool* found = NULL);
            void AddResponseHeader(const std::string& key, const std::string& value);

            std::string uri; // The URI without parameters
            std::string remote_ip; // The client ip. If the HTTP request is forwarded by NGINX we use the 'clientip' parameter in the url.   @see the NGINX conf : proxy_pass       http://127.0.0.1:8080/gasucs/special_kill/?clientip=$remote_addr;
            struct evhttp_request* req;
            Slice body;    //The http body data
            HTTPParameterMap* params;
        };

        typedef std::shared_ptr<HTTPContext> HTTPContextPtr;

        typedef std::function<
            void(const std::string& response_data)
        > HTTPSendResponseCallback;

        typedef std::function<
            void(const HTTPContextPtr& ctx,
                 const HTTPSendResponseCallback& respcb) > HTTPRequestCallback;

        typedef std::map<std::string/*The uri*/, HTTPRequestCallback> HTTPRequestCallbackMap;

        typedef std::vector<std::string> stringvector;
        typedef std::set<std::string> stringset;
    }
}