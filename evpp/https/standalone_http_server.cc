#include "standalone_http_server.h"

#include "evpp/libevent_headers.h"
#include "evpp/libevent_watcher.h"
#include "evpp/event_loop.h"
#include "evpp/event_loop_thread.h"
#include "evpp/event_loop_thread_pool.h"

namespace evpp {
    namespace https {
        class StandaloneHTTPServer::Impl {
        public:
            Impl(int thread_num);

            ~Impl();

            //! \brief Create a thread to run the http service event loop.
            //! \param[in] - int http_listening_port
            //! \param[in] - bool wait_until_http_service_started
            //!			if true : this call will be blocked until the http service has started.
            //!			if false : this call will return immediately and the http service may hasn't started.
            //! \return - bool
            bool Start(int http_listening_port, bool wait_until_http_service_started = false);

            //! \brief Stop the http server. Thread safe.
            //! \param[in] - bool wait_thread_exit : true this function will blocked until the running thread exits
            //! \return - bool - 
            bool Stop(bool wait_thread_exit = false);

            HTTPService* http_service() const;
        public:
            bool IsRunning() const;
            bool IsStopped() const;

            struct event_base* event_base() const {
                return base_loop_->event_base();
            }

            std::shared_ptr<EventLoopThreadPool> pool() const {
                return tpool_;
            }

        public:
            bool RegisterEvent(const std::string& uri,
                               HTTPRequestCallback callback);

            bool RegisterDefaultEvent(HTTPRequestCallback callback);

            void Dispatch(const HTTPContextPtr& ctx,
                          const HTTPSendResponseCallback& response_callback,
                          const HTTPRequestCallback& user_callback);

        private:
            void StopInLoop();

            void DispatchInLoop(const HTTPContextPtr& ctx,
                                const HTTPSendResponseCallback& response_callback,
                                const HTTPRequestCallback& user_callback);

        private:
            std::shared_ptr<HTTPService>   http_;
            //主要事件循环线程，监听http请求，接收HTTP请求数据和发送HTTP响应，将请求分发到工作线程
            std::shared_ptr<EventLoopThread> base_loop_;
            //工作线程池，处理请求
            std::shared_ptr<EventLoopThreadPool> tpool_;

            std::shared_ptr<PipeEventWatcher> stopping_watcher_; // 在base_loop_中工作
        };


        StandaloneHTTPServer::Impl::Impl(int thread_num)
            : base_loop_(new EventLoopThread) {
            tpool_.reset(new EventLoopThreadPool(base_loop_->event_loop(), thread_num));
            http_.reset(new HTTPService(base_loop_->event_base()));
        }

        StandaloneHTTPServer::Impl::~Impl() {
            stopping_watcher_.reset();
            http_.reset();
        }

        bool StandaloneHTTPServer::Impl::Start(int port, bool wait_for_http_service_started) {
            if (!http_->Listen(port)) {
                return false;
            }

            stopping_watcher_.reset(new PipeEventWatcher(base_loop_->event_base(), std::bind(&StandaloneHTTPServer::Impl::StopInLoop, this)));
            stopping_watcher_->Init();
            stopping_watcher_->AsyncWait();

            bool rc = tpool_->Start(wait_for_http_service_started);
            if (!rc) {
                return rc;
            }
            base_loop_->SetName("StandaloneHTTPServer-Main");

            rc = rc && base_loop_->Start(wait_for_http_service_started);

            if (wait_for_http_service_started) {
                assert(base_loop_->IsRunning());
            }
            return rc;
        }

        bool StandaloneHTTPServer::Impl::Stop(bool wait_thread_exit /*= false*/) {
            stopping_watcher_->Notify();

            if (wait_thread_exit) {
                while (!base_loop_->IsStopped() || !tpool_->IsStopped()) {
                    usleep(1);
                }
            }

            return true;
        }

        void StandaloneHTTPServer::Impl::StopInLoop() {
            //http_->Stop();
            base_loop_->event_loop()->AddAfterLoopFunctor(std::bind(&HTTPService::Stop, http_));
            tpool_->Stop(true);
            base_loop_->Stop(false); // 已经在线程base_loop的线程里了，不能调用 base_loop_->Stop(true) ， 否则相互循环等待
        }

        bool StandaloneHTTPServer::Impl::IsRunning() const {
            return base_loop_->IsRunning();
        }

        bool StandaloneHTTPServer::Impl::IsStopped() const {
            return base_loop_->IsStopped();
        }

        bool StandaloneHTTPServer::Impl::RegisterEvent(const std::string& uri, HTTPRequestCallback callback) {
            HTTPRequestCallback cb = std::bind(&Impl::Dispatch, this,
                                                std::placeholders::_1,
                                                std::placeholders::_2,
                                                callback
            );
            return http_->RegisterEvent(uri, cb);
        }

        bool StandaloneHTTPServer::Impl::RegisterDefaultEvent(HTTPRequestCallback callback) {
            HTTPRequestCallback cb = std::bind(&Impl::Dispatch, this,
                                                std::placeholders::_1,
                                                std::placeholders::_2,
                                                callback
            );
            return http_->RegisterDefaultEvent(cb);
        }

        void StandaloneHTTPServer::Impl::Dispatch(const HTTPContextPtr& ctx,
                                                  const HTTPSendResponseCallback& response_callback,
                                                  const HTTPRequestCallback& user_callback) {
            LOG_TRACE << "dispatch request " << ctx->req << " url=" << ctx->original_uri() << " in main thread";

            EventLoop* loop = tpool_->GetNextLoop();
            loop->RunInLoop(
                std::bind(&Impl::DispatchInLoop, this,
                           ctx, response_callback, user_callback));
        }

        void StandaloneHTTPServer::Impl::DispatchInLoop(const HTTPContextPtr& ctx,
                                                        const HTTPSendResponseCallback& response_callback,
                                                        const HTTPRequestCallback& user_callback) {
            LOG_TRACE << "process request " << ctx->req << " url=" << ctx->original_uri() << " in working thread";
            user_callback(ctx, response_callback);
        }

        HTTPService* StandaloneHTTPServer::Impl::http_service() const {
            return http_.get();
        }



        //////////////////////////////////////////////////////////////////////////
        //////////////////////////////////////////////////////////////////////////
        //////////////////////////////////////////////////////////////////////////
        //////////////////////////////////////////////////////////////////////////
        StandaloneHTTPServer::StandaloneHTTPServer(int thread_num) {
            impl_.reset(new Impl(thread_num));
        }

        StandaloneHTTPServer::~StandaloneHTTPServer() {}

        bool StandaloneHTTPServer::Start(int port, bool wait_for_http_service_started) {
            return impl_->Start(port, wait_for_http_service_started);
        }

        bool StandaloneHTTPServer::Stop(bool wait_thread_exit /*= false*/) {
            return impl_->Stop(wait_thread_exit);
        }

        bool StandaloneHTTPServer::IsRunning() const {
            return impl_->IsRunning();
        }

        bool StandaloneHTTPServer::IsStopped() const {
            return impl_->IsStopped();
        }

        struct event_base* StandaloneHTTPServer::event_base() const {
            return impl_->event_base();
        }

        bool StandaloneHTTPServer::RegisterEvent(const std::string& uri, HTTPRequestCallback callback) {
            return impl_->RegisterEvent(uri, callback);
        }

        bool StandaloneHTTPServer::RegisterDefaultEvent(HTTPRequestCallback callback) {
            return impl_->RegisterDefaultEvent(callback);
        }

        HTTPService* StandaloneHTTPServer::http_service() const {
            return impl_->http_service();
        }

        void StandaloneHTTPServer::set_parse_parameters(bool v) {
            http_service()->set_parse_parameters(v);
        }

        std::shared_ptr<EventLoopThreadPool> StandaloneHTTPServer::pool() const {
            return impl_->pool();
        }

    }


}