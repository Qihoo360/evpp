#include "http_server.h"

#include "evpp/libevent_headers.h"
#include "evpp/libevent_watcher.h"
#include "evpp/event_loop.h"
#include "evpp/event_loop_thread.h"
#include "evpp/event_loop_thread_pool.h"

namespace evpp {
    namespace http {
        HTTPServer::HTTPServer(int thread_num)
            : base_loop_(new EventLoopThread) {
            tpool_.reset(new EventLoopThreadPool(base_loop_->event_loop(), thread_num));
            http_.reset(new Service(base_loop_->event_base()));
        }

        HTTPServer::~HTTPServer() {
            http_.reset();
            base_loop_.reset();
            tpool_.reset();
        }

        bool HTTPServer::Start(int port, bool wait_for_http_service_started) {
            if (!http_->Listen(port)) {
                return false;
            }

            bool rc = tpool_->Start(wait_for_http_service_started);
            if (!rc) {
                return rc;
            }
            base_loop_->SetName("StandaloneHTTPServer-Main");

            // 当 base_loop_ 停止运行时，会调用该函数来停止 Service
            auto http_close_fn = std::bind(&Service::Stop, http_);
            rc = base_loop_->Start(wait_for_http_service_started,
                                   EventLoopThread::Functor(),
                                   http_close_fn);

            if (wait_for_http_service_started) {
                assert(IsRunning());
            }
            return rc;
        }

        void HTTPServer::Stop(bool wait_thread_exit /*= false*/) {
            base_loop_->Stop();
            tpool_->Stop();
            if (wait_thread_exit) {
                while (!base_loop_->IsStopped() || !tpool_->IsStopped()) {
                    usleep(1);
                }
            }
        }

//         void StandaloneHTTPServer::StopInLoop() {
//             base_loop_->Stop(false); // 已经在线程base_loop的线程里了，不能调用 base_loop_->Stop(true) ， 否则相互循环等待
//         }

        bool HTTPServer::IsRunning() const {
            return base_loop_->IsRunning() && tpool_->IsRunning();
        }

        bool HTTPServer::IsStopped() const {
            return base_loop_->IsStopped() && tpool_->IsStopped();
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