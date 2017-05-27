#include "service.h"

#include "evpp/libevent.h"
#include "evpp/event_watcher.h"
#include "evpp/event_loop.h"

namespace evpp {
namespace http {

#undef H_ARRAYSIZE
#define H_ARRAYSIZE(a) \
    ((sizeof(a) / sizeof(*(a))) / \
     static_cast<size_t>(!(sizeof(a) % sizeof(*(a)))))

static const int kMaxHTTPCode = 1000;
static const char* g_http_code_string[kMaxHTTPCode + 1];
static void InitHTTPCodeString() {
    for (size_t i = 0; i < H_ARRAYSIZE(g_http_code_string); i++) {
        g_http_code_string[i] = "XX";
    }

    g_http_code_string[200] = "OK";

    g_http_code_string[302] = "Found";

    g_http_code_string[400] = "Bad Request";
    g_http_code_string[404] = "Not Found";

    //TODO Add more http code string : https://www.w3.org/Protocols/rfc2616/rfc2616-sec10.html
}

Service::Service(EventLoop* l)
    : evhttp_(nullptr), evhttp_bound_socket_(nullptr), listen_loop_(l) {
    evhttp_ = evhttp_new(listen_loop_->event_base());
    if (!evhttp_) {
        return;
    }

    std::once_flag flag;
    std::call_once(flag, &InitHTTPCodeString);
}

Service::~Service() {
    assert(!evhttp_);
    assert(!evhttp_bound_socket_);
}

bool Service::Listen(int listen_port) {
    assert(evhttp_);
    assert(listen_loop_->IsInLoopThread());
    port_ = listen_port;

#if LIBEVENT_VERSION_NUMBER >= 0x02001500
    evhttp_bound_socket_ = evhttp_bind_socket_with_handle(evhttp_, "0.0.0.0", listen_port);
    if (!evhttp_bound_socket_) {
        return false;
    }
#else
    if (evhttp_bind_socket(evhttp_, "0.0.0.0", listen_port) != 0) {
        return false;
    }
#endif

    evhttp_set_gencb(evhttp_, &Service::GenericCallback, this);
    return true;
}

void Service::Stop() {
    DLOG_TRACE << "http service is stopping";
    assert(listen_loop_->IsInLoopThread());

    if (evhttp_) {
        evhttp_free(evhttp_);
        evhttp_ = nullptr;
        evhttp_bound_socket_ = nullptr;
    }

    callbacks_.clear();
    default_callback_ = HTTPRequestCallback();
    DLOG_TRACE << "http service stopped";
}


void Service::Pause() {
    assert(listen_loop_->IsInLoopThread());
    DLOG_TRACE << "http service pause";
#if LIBEVENT_VERSION_NUMBER >= 0x02001500
    if (evhttp_bound_socket_) {
        evconnlistener_disable(evhttp_bound_socket_get_listener(evhttp_bound_socket_));
    }
#else
    LOG_ERROR << "Not support!".;
    assert(false && "Not support");
#endif
}

void Service::Continue() {
    assert(listen_loop_->IsInLoopThread());
    DLOG_TRACE << "http service continue";
#if LIBEVENT_VERSION_NUMBER >= 0x02001500
    if (evhttp_bound_socket_) {
        evconnlistener_enable(evhttp_bound_socket_get_listener(evhttp_bound_socket_));
    }
#else
    LOG_ERROR << "Not support!".;
    assert(false && "Not support");
#endif
}

void Service::RegisterHandler(const std::string& uri, HTTPRequestCallback callback) {
    callbacks_[uri] = callback;
}

void Service::RegisterDefaultHandler(HTTPRequestCallback callback) {
    default_callback_ = callback;
}

void Service::GenericCallback(struct evhttp_request* req, void* arg) {
    Service* hsrv = static_cast<Service*>(arg);
    hsrv->HandleRequest(req);
}

void Service::HandleRequest(struct evhttp_request* req) {
    // In the main HTTP listening thread,
    // this is the main entrance of the HTTP request processing.
    assert(listen_loop_->IsInLoopThread());
    DLOG_TRACE << "handle request " << req << " url=" << req->uri;

    ContextPtr ctx(new Context(req));
    ctx->Init();

    if (callbacks_.empty()) {
        DefaultHandleRequest(ctx);
        return;
    }

    auto it = callbacks_.find(ctx->uri());
    if (it != callbacks_.end()) {
        // This will forward to HTTPServer::Dispatch method to process this request.
        auto f = std::bind(&Service::SendReply, this, ctx, std::placeholders::_1);
        it->second(listen_loop_, ctx, f);
        return;
    } else {
        DefaultHandleRequest(ctx);
    }
}

void Service::DefaultHandleRequest(const ContextPtr& ctx) {
    DLOG_TRACE << "url=" << ctx->original_uri();
    if (default_callback_) {
        auto f = std::bind(&Service::SendReply, this, ctx, std::placeholders::_1);
        default_callback_(listen_loop_, ctx, f);
    } else {
        evhttp_send_reply(ctx->req(), HTTP_BADREQUEST, g_http_code_string[HTTP_BADREQUEST], nullptr);
    }
}

struct Response {
    Response(const ContextPtr& c, const std::string& m)
        : ctx(c) {
        if (m.size() > 0) {
            buffer = evbuffer_new();
            evbuffer_add(buffer, m.c_str(), m.size());
        }
    }

    ~Response() {
        if (buffer) {
            evbuffer_free(buffer);
            buffer = nullptr;
        }

        // At this time, req is probably freed by evhttp framework.
        // So don't use req any more.
        // LOG_TRACE << "free request " << req->uri;
    }

    ContextPtr ctx;
    struct evbuffer* buffer = nullptr;
};

void Service::SendReply(const ContextPtr& ctx, const std::string& response_data) {
    // In the worker thread
    DLOG_TRACE << "send reply in working thread";

    // Build the response package in the worker thread
    std::shared_ptr<Response> response(new Response(ctx, response_data));

    auto f = [this, response]() {
        // In the main HTTP listening thread
        assert(listen_loop_->IsInLoopThread());
        DLOG_TRACE << "send reply in listening thread. evhttp_=" << evhttp_;

        auto x = response->ctx.get();

        // At this moment, this Service maybe already stopped.
        if (!evhttp_) {
            LOG_WARN << "this=" << this << " Service has been stopped.";
            return;
        }

        if (!response->buffer) {
            evhttp_send_reply(x->req(), HTTP_NOTFOUND,
                              g_http_code_string[HTTP_NOTFOUND], nullptr);
            return;
        }

        assert(x->response_http_code() <= kMaxHTTPCode);
        assert(x->response_http_code() >= 100);
        evhttp_send_reply(x->req(), x->response_http_code(),
                          g_http_code_string[x->response_http_code()],
                          response->buffer);
    };

    // Forward this response sending task to HTTP listening thread
    if (listen_loop_->IsRunning()) {
        DLOG_TRACE << "dispatch this SendReply to listening thread";
        listen_loop_->RunInLoop(f);
    } else {
        LOG_WARN << "this=" << this << " listening thread is going to stop. we discards this request.";
        // TODO do we need do some resource recycling about the evhttp_request?
    }
}
}
}
