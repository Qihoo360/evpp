#include "test_common.h"

#include <stdio.h>
#include <stdlib.h>

#include <evpp/libevent_headers.h>
#include <evpp/timestamp.h>
#include <evpp/event_loop_thread.h>

#include <evpp/httpc/conn_pool.h>
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

#if 1
TEST_UNIT(evhttpClientSample) {
    struct event_base* base = event_base_new();
    struct evhttp_connection* conn = evhttp_connection_base_new(base, NULL, "qup.f.360.cn", 80);
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

TEST_UNIT(testHTTPRequest1) {
    using namespace httpc;
    responsed = false;
    evpp::EventLoopThread t;
    t.Start(true);
    std::shared_ptr<evpp::httpc::ConnPool> pool(new evpp::httpc::ConnPool("qup.f.360.cn", 80, evpp::Duration(2.0)));
    evpp::httpc::Request r(pool.get(), t.event_loop(), "/status.html", "");
    LOG_INFO << "Do http request";
    r.Execute(std::bind(&HandleHTTPResponse, std::placeholders::_1, &t));
    while (!responsed) {
        usleep(1);
    }
    pool.reset();
    t.Stop(true);
    LOG_INFO << "EventLoopThread stopped.";
}

TEST_UNIT(testHTTPRequest2) {
    using namespace httpc;
    responsed = false;
    evpp::EventLoopThread t;
    t.Start(true);
    evpp::httpc::Request r(t.event_loop(), "http://qup.f.360.cn/status.html?a=1", "", evpp::Duration(2.0));
    LOG_INFO << "Do http request";
    r.Execute(std::bind(&HandleHTTPResponse, std::placeholders::_1, &t));
    while (!responsed) {
        usleep(1);
    }
    t.Stop(true);
    LOG_INFO << "EventLoopThread stopped.";
}

#endif
