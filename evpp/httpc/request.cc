
#include "evpp/libevent_headers.h"
#include "evpp/httpc/conn_pool.h"
#include "evpp/httpc/response.h"
#include "evpp/httpc/request.h"
#include "evpp/httpc/url_parser.h"

namespace evpp {
namespace httpc {
const std::string Request::empty_ = "";

Request::Request(ConnPool* pool, EventLoop* loop, const std::string& http_uri, const std::string& body)
    : pool_(pool), loop_(loop), host_(pool->host()), uri_(http_uri), body_(body) {
}

Request::Request(EventLoop* loop, const std::string& http_url, const std::string& body, Duration timeout)
    : pool_(nullptr), loop_(loop), body_(body) {
    //TODO performance compare
#if LIBEVENT_VERSION_NUMBER >= 0x02001500
    struct evhttp_uri* evuri = evhttp_uri_parse(http_url.c_str());
    uri_ = evhttp_uri_get_path(evuri);
    const char* query = evhttp_uri_get_query(evuri);
    if (query && strlen(query) > 0) {
        uri_ += "?";
        uri_ += query;
    }

    host_ = evhttp_uri_get_host(evuri);

    int port = evhttp_uri_get_port(evuri);
    if (port < 0) {
        port = 80;
    }

    conn_.reset(new Conn(loop, host_, port, timeout));
    evhttp_uri_free(evuri);
#else
    URLParser p(http_url);
    conn_.reset(new Conn(loop, p.host, port, timeout));
    if (p.query.empty()) {
        uri_ = p.path;
    } else {
        uri_ = p.path + "?" + p.query;
    }
    host_ = p.host;
#endif
}

Request::~Request() {
    assert(loop_->IsInLoopThread());
}

void Request::Execute(const Handler& h) {
    if (loop_->IsInLoopThread()) {
        ExecuteInLoop(h);
    } else {
        loop_->RunInLoop(std::bind(&Request::ExecuteInLoop, this, h));
    }
}

void Request::ExecuteInLoop(const Handler& h) {
    handler_ = h;

    evhttp_cmd_type req_type = EVHTTP_REQ_GET;

    std::string errmsg;
    struct evhttp_request* req = nullptr;

    if (conn_) {
        assert(pool_ == nullptr);
        if (!conn_->Init()) {
            errmsg = "conn init fail";
            goto failed;
        }
    } else {
        assert(pool_);
        conn_ = pool_->Get(loop_);
        if (!conn_->Init()) {
            errmsg = "conn init fail";
            goto failed;
        }
    }

    req = evhttp_request_new(&Request::HandleResponse, this);
    if (!req) {
        errmsg = "evhttp_request_new fail";
        goto failed;
    }

    if (evhttp_add_header(req->output_headers, "host", conn_->host().c_str())) {
        evhttp_request_free(req);
        errmsg = "evhttp_add_header failed";
        goto failed;
    }

    if (!body_.empty()) {
        req_type = EVHTTP_REQ_POST;
        if (evbuffer_add(req->output_buffer, body_.c_str(), body_.size())) {
            evhttp_request_free(req);
            errmsg = "evbuffer_add fail";
            goto failed;
        }
    }

    if (evhttp_make_request(conn_->evhttp_conn(), req, req_type, uri_.c_str())) {
        // here the conn has own the req, so don't free it twice.
        errmsg = "evhttp_make_request fail";
        goto failed;
    }

    return;

failed:
    LOG_ERROR << "http request failed: " << errmsg;
    std::shared_ptr<Response> response;
    handler_(response);
}

void Request::HandleResponse(struct evhttp_request* rsp, void* v) {
    Request* thiz = (Request*)v;
    assert(thiz);

    std::shared_ptr<Response> response;
    if (rsp) {
        response.reset(new Response(thiz, rsp));
        if (thiz->pool_) {
            thiz->pool_->Put(thiz->conn_);
        }
    } else {
        response.reset(new Response(thiz));
    }

    thiz->handler_(response);
}
} // httpc
} // evpp


