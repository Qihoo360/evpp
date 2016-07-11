#pragma once

#include "evpp/inner_pre.h"
#include "http_context.h"

#include <vector>
#include <mutex>

namespace evpp {
    class EventLoop;
    class PipeEventWatcher;
    namespace http {
        class EVPP_EXPORT Service {
        public:
            Service(EventLoop* loop);
            ~Service();

            bool Listen(int port);

            void Stop();

            // uri 不能带有参数
            bool RegisterEvent(const std::string& uri, HTTPRequestCallback callback);

            bool RegisterDefaultEvent(HTTPRequestCallback callback);

        private:
            void HandleRequest(struct evhttp_request *req);

            void DefaultHandleRequest(const HTTPContextPtr& ctx);

        private:
            static void GenericCallback(struct evhttp_request *req, void *arg);

        private:
            void SendReply(struct evhttp_request *req, const std::string& response);

        private:
            struct evhttp* evhttp_;
            EventLoop* loop_;
            HTTPRequestCallbackMap callbacks_;
            HTTPRequestCallback    default_callback_;
        };
    }

}