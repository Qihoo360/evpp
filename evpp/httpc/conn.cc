#include "evpp/httpc/conn.h"
#include "evpp/httpc/conn_pool.h"

#if defined(EVPP_HTTP_CLIENT_SUPPORTS_SSL)
#include "evpp/httpc/ssl.h"
#include <openssl/x509v3.h>
#include <openssl/err.h>
#endif

#include "evpp/libevent.h"

namespace evpp {
namespace httpc {
Conn::Conn(ConnPool* p, EventLoop* l)
    : loop_(l), pool_(p)
    , host_(p->host())
    , port_(p->port())
#if defined(EVPP_HTTP_CLIENT_SUPPORTS_SSL)
    , enable_ssl_(p->enable_ssl())
    , ssl_(nullptr)
    , bufferevent_(nullptr)
#endif
    , timeout_(p->timeout())
    , evhttp_conn_(nullptr) {
}

Conn::Conn(EventLoop* l, const std::string& h, int p,
#if defined(EVPP_HTTP_CLIENT_SUPPORTS_SSL)
    bool enable_ssl,
#endif
    Duration t)
    : loop_(l), pool_(nullptr)
    , host_(h)
    , port_(p)
#if defined(EVPP_HTTP_CLIENT_SUPPORTS_SSL)
    , enable_ssl_(enable_ssl)
    , ssl_(nullptr)
    , bufferevent_(nullptr)
#endif
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

#if defined(EVPP_HTTP_CLIENT_SUPPORTS_SSL)
    if (enable_ssl()) {
        ssl_ = SSL_new(GetSSLCtx());
        if (!ssl_) {
            LOG_ERROR << "SSL_new failed.";
            return false;
        }
        SSL_set_tlsext_host_name(ssl_, host_.c_str());
        X509_VERIFY_PARAM* param = SSL_get0_param(ssl_);
        X509_VERIFY_PARAM_set_hostflags(param, X509_CHECK_FLAG_NO_PARTIAL_WILDCARDS);
        X509_VERIFY_PARAM_set1_host(param, host_.c_str(), 0);
        SSL_set_verify(ssl_, SSL_VERIFY_PEER, nullptr);
        bufferevent_ = bufferevent_openssl_socket_new(loop_->event_base(), -1, ssl_,
            BUFFEREVENT_SSL_CONNECTING,
            BEV_OPT_CLOSE_ON_FREE | BEV_OPT_DEFER_CALLBACKS);
    } else {
        bufferevent_ = bufferevent_socket_new(loop_->event_base(), -1, BEV_OPT_CLOSE_ON_FREE);
    }
    if (!bufferevent_) {
        LOG_ERROR << "bufferevent creation failed.";
        return false;
    }
    bufferevent_openssl_set_allow_dirty_shutdown(bufferevent_, 1);
    evhttp_conn_ = evhttp_connection_base_bufferevent_new(loop_->event_base(), NULL, bufferevent_, host_.c_str(), port_);
#else
    evhttp_conn_ = evhttp_connection_base_new(loop_->event_base(), nullptr, host_.c_str(), port_);
#endif
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
#if defined(EVPP_HTTP_CLIENT_SUPPORTS_SSL)
    // ssl_ gets freed by be_openssl_destruct because of BEV_OPT_CLOSE_ON_FREE
    ssl_ = nullptr;
#endif
}
} // httpc
} // evpp
