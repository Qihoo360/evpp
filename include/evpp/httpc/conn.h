#pragma once

#include "evpp/inner_pre.h"
#include "evpp/event_loop.h"

struct evhttp_connection;
namespace evpp {
    namespace httpc {
        class Pool;
        class EVPP_EXPORT Conn {
        public:
            Conn(Pool* pool, EventLoop* loop);
            ~Conn();

            bool Init();
            void Close();

            Pool* pool() { return pool_; }
            EventLoop* loop() { return loop_; }
            struct evhttp_connection* evhttp_conn() { return evhttp_conn_; }
        private:
            Pool* pool_;
            EventLoop* loop_;
            struct evhttp_connection* evhttp_conn_;
        };
    } // httpc
} // evpp