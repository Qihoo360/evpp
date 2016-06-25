#pragma once

#include "evpp/inner_pre.h"
#include "evpp/event_loop.h"

struct evhttp_connection;
namespace evpp {
    namespace httpc {
        class Pool;
        class Response;
        class Conn;
        typedef std::function<void(const std::shared_ptr<Response>&)> Handler;

        class EVPP_EXPORT Request {
        public:
            Request(Pool* pool, EventLoop* loop, const std::string& uri, const std::string& body);
            ~Request();

            void Execute(const Handler& h);

            Pool* pool() { return pool_; }
            EventLoop* loop() { return loop_; }
        private:
            static void Request::HandleResponse(struct evhttp_request* r, void *v);
            void ExecuteInLoop(const Handler& h);
        private:
            Pool* pool_;
            EventLoop* loop_;
            std::string uri_;
            std::string body_;
            std::shared_ptr<Conn> conn_;
            Handler handler_;
        };
    } // httpc
} // evpp