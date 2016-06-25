#include "evpp/httpc/conn.h"
#include "evpp/httpc/pool.h"

#include "evpp/libevent_headers.h"

namespace evpp {
    namespace httpc {
        Conn::Conn(Pool* p, EventLoop* l) : pool_(p), loop_(l), evhttp_conn_(NULL) {
        }

        Conn::~Conn() {
            Close();
        }

        bool Conn::Init() {
            if (evhttp_conn_) {
                return true;
            }

            evhttp_conn_ = evhttp_connection_new(pool_->host().c_str(), pool_->port());
            if (!evhttp_conn_) {
                LOG_ERROR << "evhttp_connection_new failed.";
                return false;
            }

            evhttp_connection_set_base(evhttp_conn_, loop_->event_base());

            if (!pool_->timeout().IsZero()) {
#if LIBEVENT_VERSION_NUMBER >= 0x02010500
                struct timeval tv = pool_->timeout().TimeVal();
                evhttp_connection_set_timeout_tv(evhttp_conn_, &tv);
#else
                double timeout_sec = pool_->timeout().Seconds();
                if (timeout_sec < 1.0) {
                    timeout_sec = 1.0;
                }
                evhttp_connection_set_timeout(evhttp_conn_, int(timeout_sec));
#endif
            }
        }

        void Conn::Close() {
            if (evhttp_conn_) {
                evhttp_connection_free(evhttp_conn_);
                evhttp_conn_ = NULL;
            }
        }

} // httpc
} // evpp