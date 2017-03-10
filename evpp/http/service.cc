#include "service.h"

#include "evpp/libevent_headers.h"
#include "evpp/libevent_watcher.h"
#include "evpp/event_loop.h"

namespace evpp {
namespace http {
Service::Service(EventLoop* l)
    : evhttp_(nullptr), evhttp_bound_socket_(nullptr), listen_loop_(l) {
    evhttp_ = evhttp_new(listen_loop_->event_base());
    if (!evhttp_) {
        return;
    }
}

Service::~Service() {
    assert(!evhttp_);
    assert(!listen_loop_);
}

bool Service::Listen(int port) {
    assert(evhttp_);
    assert(listen_loop_->IsInLoopThread());

#if LIBEVENT_VERSION_NUMBER >= 0x02001500
    evhttp_bound_socket_ = evhttp_bind_socket_with_handle(evhttp_, "0.0.0.0", port);
    if (!evhttp_bound_socket_) {
        return false;
    }
#else
    if (evhttp_bind_socket(evhttp_, "0.0.0.0", port) != 0) {
        return false;
    }
#endif

    evhttp_set_gencb(evhttp_, &Service::GenericCallback, this);
    return true;
}

void Service::Stop() {
    assert(listen_loop_->IsInLoopThread());

    if (evhttp_) {
        evhttp_free(evhttp_);
        evhttp_ = nullptr;
    }

    listen_loop_ = nullptr;
    callbacks_.clear();
    default_callback_ = HTTPRequestCallback();
}


void Service::Pause() {
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
    LOG_TRACE << "handle request " << req << " url=" << req->uri;

    ContextPtr ctx(new Context(req));
    ctx->Init();

    if (callbacks_.empty()) {
        DefaultHandleRequest(ctx);
        return;
    }

    auto it = callbacks_.find(ctx->uri());
    if (it != callbacks_.end()) {
        // This will forward to HTTPServer::Dispatch method to process this request.
        auto f = std::bind(&Service::SendReply, this, req, std::placeholders::_1);
        it->second(listen_loop_, ctx, f);
        return;
    } else {
        DefaultHandleRequest(ctx);
    }
}

void Service::DefaultHandleRequest(const ContextPtr& ctx) {
    if (default_callback_) {
        auto f = std::bind(&Service::SendReply, this, ctx->req(), std::placeholders::_1);
        default_callback_(listen_loop_, ctx, f);
    } else {
        evhttp_send_reply(ctx->req(), HTTP_BADREQUEST, "Bad Request", nullptr);
    }
}

struct Response {
    Response(struct evhttp_request* r, const std::string& m)
        : req(r), buffer(nullptr) {
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

        LOG_TRACE << "free request " << req->uri;
    }

    struct evhttp_request*  req;
    struct evbuffer* buffer;
};

void Service::SendReply(struct evhttp_request* req, const std::string& response_data) {
    // In the worker thread
    LOG_TRACE << "send reply in working thread";

    // Build the response package in the worker thread
    std::shared_ptr<Response> response(new Response(req, response_data));

    auto f = [this, response]() {
        // In the main HTTP listening thread
        assert(listen_loop_->IsInLoopThread());
        LOG_TRACE << "send http reply";

        if (!response->buffer) {
            evhttp_send_reply(response->req, HTTP_NOTFOUND, "Not Found", nullptr);
            return;
        }

        evhttp_send_reply(response->req, HTTP_OK, "OK", response->buffer);
    };

    // Forward this response sending task to HTTP listening thread
    listen_loop_->RunInLoop(f);
}
}
}
