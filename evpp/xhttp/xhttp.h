#pragma once

#include <string>
#include <map>
#include <string.h>
#include <memory>
#include <functional>


extern "C"
{
struct http_parser;    
}

namespace evpp
{
namespace xhttp {
    class HttpConnection;
    class WsConnection;
    class HttpRequest;
    class HttpResponse;

    class HttpParser;
    class HttpClientConn;
    class HttpClient;
    class WsClient;
    class HttpConnector;

    //we use 599 for our tnet error

    const int TNET_HTTP_ERROR = 599;

    class HttpError
    {
    public:
        HttpError(int code = 200, const std::string& m = std::string())
            : statusCode(code)
            , message(m)
        {}    

        //200 for no error
        int statusCode;
        std::string message;
    };
    
    enum WsEvent
    { 
        Ws_OpenEvent,
        Ws_CloseEvent, 
        Ws_MessageEvent,
        Ws_PongEvent,    
        Ws_ErrorEvent,
    };

    typedef std::shared_ptr<HttpConnection> HttpConnectionPtr_t;
    typedef std::weak_ptr<HttpConnection> WeakHttpConnectionPtr_t;

    typedef std::shared_ptr<WsConnection> WsConnectionPtr_t;
    typedef std::weak_ptr<HttpConnection> WeakWsConnectionPtr_t;

    typedef std::shared_ptr<HttpParser> HttpParser_t;
    typedef std::shared_ptr<HttpClientConn> HttpClientConnPtr_t;
    typedef std::shared_ptr<HttpConnector> HttpConnectorPtr_t;
    typedef std::weak_ptr<HttpConnector> WeakHttpConnectorPtr_t;

    typedef std::shared_ptr<HttpClient> HttpClientPtr_t;
    typedef std::shared_ptr<WsClient> WsClientPtr_t;

    enum RequestEvent
    {
        Request_Upgrade, 
        Request_Complete,
        Request_Error,   
    };
    
    //Request_Upgrade: context is &StackBuffer
    //Request_Complete: context is 0
    //Request_Error: context is &HttpError

    enum ResponseEvent
    {
        Response_Complete,
        Response_Error,    
    };

    //Response_Complete: context is 0
    //Response_Error: context is &HttpError

    typedef std::function<void (void)> Callback_t;
    typedef std::function<void (const HttpConnectionPtr_t&, const HttpRequest&)> HttpCallback_t;
    
    //Ws_OpenEvent: server side, context is &HttpRequest, client side, context is &HttpResponse
    //Ws_MessageEvent: context is &std::string
    //Other Ws Event: context is 0
    
    typedef std::function<void (const WsConnectionPtr_t&, WsEvent, const void*)> WsCallback_t;

    typedef std::function<void (const HttpResponse&)> ResponseCallback_t;

    typedef std::function<HttpError (const HttpRequest&)> AuthCallback_t;

    struct CaseKeyCmp
    {
        bool operator() (const std::string& p1, const std::string& p2) const
        {
            return strcasecmp(p1.c_str(), p2.c_str()) < 0;
        }    
    };

    typedef std::multimap<std::string, std::string, CaseKeyCmp> Headers_t;
    typedef std::multimap<std::string, std::string> Params_t;
}
}