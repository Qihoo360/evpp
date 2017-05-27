#include "evpp/httpc/conn.h"
#include "evpp/httpc/conn_pool.h"

#include "evpp/libevent.h"

namespace evpp {
namespace httpc {
Conn::Conn(ConnPool* p, EventLoop* l)
    : loop_(l), pool_(p)
    , host_(p->host())
    , port_(p->port())
    , timeout_(p->timeout())
    , evhttp_conn_(nullptr) {
}

Conn::Conn(EventLoop* l, const std::string& h, int p, Duration t)
    : loop_(l), pool_(nullptr)
    , host_(h)
    , port_(p)
    , timeout_(t)
    , evhttp_conn_(nullptr) {
}

Conn::~Conn() {
    Close();
}

bool Conn::Init() {
    if (evhttp_conn_) {
        return true;
    }

    evhttp_conn_ = evhttp_connection_base_new(loop_->event_base(), nullptr, host_.c_str(), port_);
    if (!evhttp_conn_) {
        LOG_ERROR << "evhttp_connection_new failed.";
        return false;
    }

    if (!timeout_.IsZero()) {
#if LIBEVENT_VERSION_NUMBER >= 0x02010500
        struct timeval tv = timeout_.TimeVal();
        evhttp_connection_set_timeout_tv(evhttp_conn_, &tv);
#else
        double timeout_sec = timeout_.Seconds();
        if (timeout_sec < 1.0) {
            timeout_sec = 1.0;
        }
        evhttp_connection_set_timeout(evhttp_conn_, int(timeout_sec));
#endif
    }

    return true;
}

void Conn::Close() {
    if (evhttp_conn_) {
        assert(loop_->IsInLoopThread());
        evhttp_connection_free(evhttp_conn_);
        evhttp_conn_ = nullptr;
    }
}
} // httpc
} // evpp
