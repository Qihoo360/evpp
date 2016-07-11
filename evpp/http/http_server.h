#pragma once

#include "service.h"

struct event_base;
struct event;

namespace evpp {
    class EventLoopThreadPool;
    class PipeEventWatcher;
    class EventLoopThread;

    namespace http {
        class Service;

        // ����һ�����Զ������е� HTTP Server
        // ��������һ���������߳����ڶ˿ڼ���������HTTP���󡢷ַ�HTTP���������HTTP��Ӧ��
        // ��� thread_num ��Ϊ 0������������һ���̳߳أ����ڴ���HTTP����
        class EVPP_EXPORT HTTPServer {
        public:
            HTTPServer(int thread_num = 0);

            ~HTTPServer();

            //! \brief Create a thread to run the http service event loop.
            //! \param[in] - int http_listening_port
            //! \param[in] - bool wait_until_http_service_started
            //!			if true : this call will be blocked until the http service has started.
            //!			if false : this call will return immediately and the http service may start later asynchronously.
            //! \return - bool
            bool Start(int http_listening_port, bool wait_until_http_service_started = false);

            //! \brief Stop the http server. Thread safe.
            //! \param[in] - bool wait_thread_exit 
            //!     true this function will blocked until the running thread exits
            //! \return - bool - 
            void Stop(bool wait_thread_exit = false);

            Service* http_service() const;
        public:
            bool RegisterEvent(const std::string& uri,
                               HTTPRequestCallback callback);

            bool RegisterDefaultEvent(HTTPRequestCallback callback);

        public:
            bool IsRunning() const;
            bool IsStopped() const;

            std::shared_ptr<EventLoopThreadPool> pool() const { return tpool_; }

        private:
            void Dispatch(const HTTPContextPtr& ctx,
                          const HTTPSendResponseCallback& response_callback,
                          const HTTPRequestCallback& user_callback);

        private:
            std::shared_ptr<Service>   http_;

            // ��Ҫ�¼�ѭ���̣߳�����http���󣬽���HTTP�������ݺͷ���HTTP��Ӧ��������ַ��������߳�
            std::shared_ptr<EventLoopThread> base_loop_;

            // �����̳߳أ���������
            std::shared_ptr<EventLoopThreadPool> tpool_;
        };
    }

}