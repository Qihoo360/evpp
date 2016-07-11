#include "http_server.h"

#include "evpp/libevent_headers.h"
#include "evpp/libevent_watcher.h"
#include "evpp/event_loop.h"
#include "evpp/event_loop_thread.h"
#include "evpp/event_loop_thread_pool.h"

namespace evpp {
    namespace http {
        HTTPServer::HTTPServer(int thread_num)
            : listen_thread_(new EventLoopThread) {
            tpool_.reset(new EventLoopThreadPool(listen_thread_->event_loop(), thread_num));
            http_.reset(new Service(listen_thread_->event_base()));
        }

        HTTPServer::~HTTPServer() {
            http_.reset();
            listen_thread_.reset();
            tpool_.reset();
        }

        bool HTTPServer::Start(int port) {
            if (!http_->Listen(port)) {
                return false;
            }

            bool rc = tpool_->Start(true);
            if (!rc) {
                return rc;
            }
            listen_thread_->SetName("StandaloneHTTPServer-Main");

            // 当 base_loop_ 停止运行时，会调用该函数来停止 Service
            auto http_close_fn = std::bind(&Service::Stop, http_);
            rc = listen_thread_->Start(true,
                                   EventLoopThread::Functor(),
                                   http_close_fn);

            while (!IsRunning()) {
                usleep(1);
            }
            return rc;
        }

        void HTTPServer::Stop(bool wait_thread_exit /*= false*/) {
            listen_thread_->Stop();
            tpool_->Stop();
            if (wait_thread_exit) {
                while (!listen_thread_->IsStopped() || !tpool_->IsStopped()) {
                    usleep(1);
                }
            }
        }

        bool HTTPServer::IsRunning() const {
            return listen_thread_->IsRunning() && tpool_->IsRunning();
        }

        bool HTTPServer::IsStopped() const {
            return listen_thread_->IsStopped() && tpool_->IsStopped();
        }

        bool HTTPServer::RegisterEvent(const std::string& uri, HTTPRequestCallback callback) {
            HTTPRequestCallback cb = std::bind(&HTTPServer::Dispatch, this,
                                               std::placeholders::_1,
                                               std::placeholders::_2,
                                               callback);
            return http_->RegisterEvent(uri, cb);
        }

        bool HTTPServer::RegisterDefaultEvent(HTTPRequestCallback callback) {
            HTTPRequestCallback cb = std::bind(&HTTPServer::Dispatch, this,
                                               std::placeholders::_1,
                                               std::placeholders::_2,
                                               callback);
            return http_->RegisterDefaultEvent(cb);
        }

        void HTTPServer::Dispatch(const HTTPContextPtr& ctx,
                                            const HTTPSendResponseCallback& response_callback,
                                            const HTTPRequestCallback& user_callback) {
            LOG_TRACE << "dispatch request " << ctx->req << " url=" << ctx->original_uri() << " in main thread";

            // 将该HTTP请求调度到工作线程处理
            auto f = [](const HTTPContextPtr& ctx,
                        const HTTPSendResponseCallback& response_callback,
                        const HTTPRequestCallback& user_callback) {
                LOG_TRACE << "process request " << ctx->req << " url=" << ctx->original_uri() << " in working thread";
                user_callback(ctx, response_callback);
            };
            EventLoop* loop = tpool_->GetNextLoop();
            loop->RunInLoop(std::bind(f, ctx, response_callback, user_callback));
        }

        Service* HTTPServer::http_service() const {
            return http_.get();
        }
    }
}