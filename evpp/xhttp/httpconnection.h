#pragma once

#include <map>
#include <string>

extern "C"
{
#include "http_parser.h"
}

#include "xhttp.h"
#include "httpparser.h"
#include "httprequest.h"
#include <evpp/tcp_conn.h>
#include <evpp/tcp_callbacks.h>

namespace evpp
{
namespace xhttp { 
    class HttpServer;
    class HttpRequest;
    class HttpResponse;

    //http server connection
    class HttpConnection : public HttpParser
                     , public std::enable_shared_from_this<HttpConnection> 
    {
    public:
        friend class HttpServer;

        typedef std::function<void (HttpConnectionPtr_t, const HttpRequest&, RequestEvent, void*)> RequestCallback_t;
        
        HttpConnection(const evpp::TCPConnPtr& conn, const RequestCallback_t& callback);

        ~HttpConnection();

        //int getSockFd() { return m_fd; }

        void send(HttpResponse& resp);
        void send(int statusCode);
        void send(int statusCode, const std::string& body);
        void send(int statusCode, const std::string& body, const Headers_t& headers);

        //send completely callback, called when all send buffers are send.
        //If there was a previous callback, that callback will be overwritten
        void send(HttpResponse& resp, const Callback_t& callback);
        void send(int statusCode, const Callback_t& callback);
        void send(int statusCode, const std::string& body, const Callback_t& callback);
        void send(int statusCode, const std::string& body, const Headers_t& headers, const Callback_t& callback);

        //after is milliseconds
        void shutDown(int after);

        //ConnectionPtr_t lockConn() { return m_conn.lock(); }
        //WeakConnectionPtr_t getConn() { return m_conn; }

        static void setMaxHeaderSize(size_t size) { ms_maxHeaderSize = size; }
        static void setMaxBodySize(size_t size) { ms_maxBodySize = size; }
        
    protected:

        void onDataReceived(const TCPConnPtr& conn, evpp::Buffer* buffer);
    private:
        // http parser
        int onMessageBegin();
        int onUrl(const char* at, size_t length);
        int onHeader(const std::string& field, const std::string& value);
        int onHeadersComplete();
        int onBody(const char* at, size_t length);
        int onMessageComplete();
        int onUpgrade(const char* at, size_t length);
        int onError(HttpError& error);

        void onTcpWriteComplete(const TCPConnPtr&);        
    private:
        std::weak_ptr<evpp::TCPConn> m_conn;
         
        HttpRequest m_request;    
    
        RequestCallback_t m_callback;

        Callback_t m_sendCallback;

        static size_t ms_maxHeaderSize;
        static size_t ms_maxBodySize;
    };    
}
}
