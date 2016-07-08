#pragma once

#include "embedded_http_server.h"

struct event_base;
struct event;

namespace evpp {
    class EventLoopThreadPool;
    namespace https {
        class HTTPService;
        class EVPP_EXPORT StandaloneHTTPServer {
        public:
            StandaloneHTTPServer(int thread_num = 0);

            ~StandaloneHTTPServer();

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
            bool Stop(bool wait_thread_exit = false);

            HTTPService* http_service() const;
            void set_parse_parameters(bool v);
        public:
            bool RegisterEvent(const std::string& uri,
                               HTTPRequestCallback callback);

            bool RegisterDefaultEvent(HTTPRequestCallback callback);

        public:
            bool IsRunning() const;
            bool IsStopped() const;

            struct event_base* event_base() const;
            std::shared_ptr<EventLoopThreadPool> pool() const;
        private:
            class Impl;
            std::shared_ptr<Impl> impl_;
        };
    }

}