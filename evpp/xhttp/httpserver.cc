#include "httpserver.h"

#include <string>
#include <evpp/event_loop.h>
#include <evpp/tcp_server.h>
#include <evpp/logging.h>
#include <evpp/buffer.h>
#include <evpp/any.h>

#include "httpconnection.h"
#include "httprequest.h"
#include "httpresponse.h"
#include "httpparser.h"



using namespace std;

using namespace evpp;

using std::placeholders::_1;
using std::placeholders::_2;
using std::placeholders::_3;
using std::placeholders::_4;
using std::placeholders::_5;

namespace evpp
{
namespace xhttp { 
    static string rootPath = "/";

    static void httpNotFoundCallback(const HttpConnectionPtr_t& conn, const HttpRequest& request)
    {
        HttpResponse resp;
        resp.statusCode = 404;
        
        conn->send(resp);      
    } 


    HttpServer::HttpServer(evpp::EventLoop* loop, 
            const std::string& listen_addr /* ip:port */,
            const std::string& name, 
            int thread_num)
    : m_loop(loop)
    , m_server(nullptr)
    , m_serverName(name)
    , m_listenAddr(listen_addr)
    , m_threadNum(thread_num)
    , m_defaultCallback(httpNotFoundCallback)  {

    }
   
    HttpServer::~HttpServer()  {
        if (m_server != nullptr) {
            delete m_server;
            m_server = nullptr;
        }
        
    }
     
    bool HttpServer::Init() {
        if (m_server == nullptr) {
            m_server = new evpp::TCPServer(m_loop, m_listenAddr, m_serverName, m_threadNum);
            m_server->SetConnectionCallback(std::bind(&HttpServer::onTcpConnectionCallback, this, std::placeholders::_1));
            m_server->SetMessageCallback(std::bind(&HttpServer::onTcpMessageCallback, this, std::placeholders::_1, std::placeholders::_2));
            return m_server->Init();
        }
        return false;
    }

    bool HttpServer::Start() {
        if (m_server)
            return m_server->Start();
        return false;
    }

    bool HttpServer::Stop() {
        // FIXME: wait....
        m_server->Stop();
        return true;
    }


    
    void HttpServer::setHttpCallback(const string& path, const HttpCallback_t& callback)
    {
        m_httpCallbacks[path] = callback;    
    }

    void HttpServer::onTcpConnectionCallback(const TCPConnPtr& conn) {
        if (conn->IsConnected()) {
            HttpConnectionPtr_t httpConn = std::make_shared<HttpConnection>(conn, 
                        std::bind(&HttpServer::onRequest, this, _1, _2, _3, _4));
            
            conn->set_context(evpp::Any(httpConn));
        }
        else {
            conn->set_context(evpp::Any(nullptr));
        }
    }
    
    
    void HttpServer::onTcpMessageCallback(const TCPConnPtr& conn, evpp::Buffer* buffer) {
        HttpConnectionPtr_t httpConn = evpp::any_cast<HttpConnectionPtr_t>(conn->context());
        httpConn->onDataReceived(conn, buffer);
    }

    void HttpServer::onRequest(const HttpConnectionPtr_t& conn, const HttpRequest& request, RequestEvent event, const void* context) {

        switch(event)
        {
            case Request_Upgrade:
                //onWebsocket(conn, request, context);
                break;
            case Request_Error:
                //onError(conn, *(HttpError*)context);
                break;
            case Request_Complete:
                {
                    map<string, HttpCallback_t>::iterator iter = m_httpCallbacks.find(request.path);
                    if(iter == m_httpCallbacks.end())
                    {
                        //httpNotFoundCallback(conn, request); 
                        m_defaultCallback(conn, request);
                    }
                    else
                    {
                        //if(authRequest(conn, request))
                        { 
                            (iter->second)(conn, request);    
                        }
                    }
                }
                break;
            default:
                LOG_ERROR << "invalid request event" << event;
                break;
        }
    }

    void HttpServer::setHttpDefaultCallback(const HttpCallback_t& callback) {
        if (callback) {
            m_defaultCallback = callback;
        }
        else {
            m_defaultCallback = httpNotFoundCallback;
        }
    }

}
}
