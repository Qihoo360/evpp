#pragma once

#include "evpp/inner_pre.h"
#include "evpp/event_loop.h"

struct evhttp_connection;
namespace evpp {
    namespace httpc {
        class ConnPool;
        class Response;
        class Conn;
        typedef std::function<void(const std::shared_ptr<Response>&)> Handler;

        class EVPP_EXPORT Request {
        public:
            //! \brief Create a HTTP Request and create Conn from pool.
            //! Do a HTTP GET request if body is empty or HTTP POST request if body is not empty.
            //! \param[in] - ConnPool * pool
            //! \param[in] - EventLoop * loop
            //! \param[in] - const std::string & uri The URI of the HTTP request
            //! \param[in] - const std::string & body
            //! \return - 
            Request(ConnPool* pool, EventLoop* loop, const std::string& uri, const std::string& body);
            
            //! \brief Create a HTTP Request and create Conn myself
            //! \param[in] - EventLoop * loop
            //! \param[in] - const std::string & url The URL of the HTTP request
            //! \param[in] - const std::string & body
            //! \param[in] - Duration timeout
            //! \return - 
            //! \return - 
            Request(EventLoop* loop, const std::string& url, const std::string& body, Duration timeout);
            ~Request();

            void Execute(const Handler& h);

            //ConnPool* pool() { return pool_; }
            //EventLoop* loop() { return loop_; }
        private:
            static void Request::HandleResponse(struct evhttp_request* r, void *v);
            void ExecuteInLoop(const Handler& h);
        private:
            ConnPool* pool_;
            EventLoop* loop_;
            std::string uri_;
            std::string body_;
            std::shared_ptr<Conn> conn_;
            Handler handler_;
        };
    } // httpc
} // evpp