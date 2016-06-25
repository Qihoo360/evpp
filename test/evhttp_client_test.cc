#include "test_common.h"

#include <stdio.h>
#include <stdlib.h>

#include <evpp/libevent_headers.h>
#include <evpp/timestamp.h>
#include <evpp/event_loop_thread.h>

#include <evpp/httpc/pool.h>
#include <evpp/httpc/request.h>
#include <evpp/httpc/conn.h>
#include <evpp/httpc/response.h>

namespace {
    void http_request_done(struct evhttp_request *req, void *arg) {
        char buf[1024] = {};
        evbuffer_remove(req->input_buffer, &buf, sizeof(buf) - 1);
        printf("%s", buf);
        event_base_loopbreak((struct event_base *)arg);
    }
}

TEST_UNIT(evhttpClientSample) {
    struct event_base* base = event_base_new();
    struct evhttp_connection* conn = evhttp_connection_new("qup.f.360.cn", 80);
    evhttp_connection_set_base(conn, base);
    struct evhttp_request* req = evhttp_request_new(http_request_done, base);
    evhttp_add_header(req->output_headers, "Host", "qup.f.360.cn");
    evhttp_make_request(conn, req, EVHTTP_REQ_GET, "/status.html");
    evhttp_connection_set_timeout(req->evcon, 600);
    event_base_dispatch(base);
}

namespace httpc {
    static bool responsed = false;
    static void HandleHTTPResponse(const std::shared_ptr<evpp::httpc::Response>& r, evpp::EventLoopThread* t) {
        LOG_INFO << "http_code=" << r->http_code() << " [" << r->body().ToString() << "]";
        responsed = true;
    }
}

TEST_UNIT(testHTTPRequest) {
    using namespace httpc;
    evpp::EventLoopThread t;
    t.Start(true);
    std::shared_ptr<evpp::httpc::Pool> pool(new evpp::httpc::Pool("qup.f.360.cn", 80, evpp::Duration(2.0)));
    evpp::httpc::Request r(pool.get(), t.event_loop(), "/status.html", "");
    r.Execute(std::bind(&HandleHTTPResponse, std::placeholders::_1, &t));
    while (!responsed) {
        usleep(1);
    }
    pool.reset();
    t.Stop(true);
    LOG_INFO << "EventLoopThread stopped.";
}

