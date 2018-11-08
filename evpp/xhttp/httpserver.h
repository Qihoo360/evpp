#pragma once

#include <functional>
#include <memory>
#include <stdint.h>
#include <map>

#include <evpp/tcp_callbacks.h>


#include "xhttp.h"

extern "C"
{
#include "http_parser.h"    
}

namespace evpp
{
class EventLoop;
class TCPServer;
class Buffer;


namespace xhttp { 

    class HttpServer //: public nocopyable
    {
    public:
        friend class HttpConnection;

        HttpServer(evpp::EventLoop* loop, 
            const std::string& listen_addr /* ip:port */,
            const std::string& name, 
            int thread_num);
        ~HttpServer();

        bool Init();

        bool Start();

        bool Stop();
    
        void setHttpCallback(const std::string& path, const HttpCallback_t& callback);
        void setHttpDefaultCallback(const HttpCallback_t& callback);
    protected:
        void onTcpConnectionCallback(const TCPConnPtr& conn);
        void onTcpMessageCallback(const TCPConnPtr&, evpp::Buffer*);

        void onRequest(const HttpConnectionPtr_t& conn, const HttpRequest& request, RequestEvent event, const void* context);

        void onHttpRequestError(const HttpConnectionPtr_t& conn, const HttpRequest& request, const HttpError& error);
        void onWebSocket(const HttpConnectionPtr_t& conn, const HttpRequest& request, evpp::Buffer* buffer);
    private:
        evpp::EventLoop* m_loop;
        evpp::TCPServer* m_server;
        std::string m_serverName; 
        std::string m_listenAddr;
        int m_threadNum;
    
        std::map<std::string, HttpCallback_t> m_httpCallbacks;  
        HttpCallback_t m_defaultCallback;      

    };
    
}
}

