#pragma once

#include "evpp/inner_pre.h"
#include "evpp/event_loop.h"

#include "evpp/httpc/conn.h"

struct evhttp_connection;
namespace evpp {
    namespace httpc {
        class ConnPool;
        class Response;
        class Conn;
        typedef std::function<void(const std::shared_ptr<Response>&)> Handler;

        class EVPP_EXPORT Request {
        public:
            // @brief Create a HTTP Request and create Conn from pool.
            //  Do a HTTP GET request if body is empty or HTTP POST request if body is not empty.
            // @param[in] pool - 
            // @param[in] loop - 
            // @param[in] uri - The URI of the HTTP request
            // @param[in] body - 
            // @return  - 
            Request(ConnPool* pool, EventLoop* loop, const std::string& uri, const std::string& body);
            
            // @brief Create a HTTP Request and create Conn myself
            //  Do a HTTP GET request if body is empty or HTTP POST request if body is not empty.
            // @param[in] loop - 
            // @param[in] url - The URL of the HTTP request
            // @param[in] body - 
            // @param[in] timeout - 
            // @return - 
            Request(EventLoop* loop, const std::string& url, const std::string& body, Duration timeout);

            ~Request();

            void Execute(const Handler& h);

            const std::shared_ptr<Conn>& conn() const { return conn_; }
            struct evhttp_connection* evhttp_conn() const { return conn_->evhttp_conn(); }
            const std::string& uri() const { return uri_; }
        private:
            static void HandleResponse(struct evhttp_request* r, void *v);
            void ExecuteInLoop(const Handler& h);
        private:
            ConnPool* pool_;
            EventLoop* loop_;
            std::string uri_;
            std::string body_;
            std::shared_ptr<Conn> conn_;
            Handler handler_;
        };
        typedef std::shared_ptr<Request> RequestPtr;
    } // httpc
} // evpp
