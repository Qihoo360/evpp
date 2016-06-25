#include "test_common.h"

#include <stdio.h>
#include <stdlib.h>

#include <evpp/libevent_headers.h>

namespace {
    void http_request_done(struct evhttp_request *req, void *arg) {
        char buf[1024] = {};
        evbuffer_remove(req->input_buffer, &buf, sizeof(buf) - 1);
        printf("%s", buf);
        event_base_loopbreak((struct event_base *)arg);
    }
}

TEST_UNIT(evhttpClientSample) {
    struct event_base *base;
    struct evhttp_connection *conn;
    struct evhttp_request *req;
    base = event_base_new();
    conn = evhttp_connection_new("qup.f.360.cn", 80);
    evhttp_connection_set_base(conn, base);
    req = evhttp_request_new(http_request_done, base);
    evhttp_add_header(req->output_headers, "Host", "qup.f.360.cn");
    evhttp_make_request(conn, req, EVHTTP_REQ_GET, "/status.html");
    evhttp_connection_set_timeout(req->evcon, 600);
    event_base_dispatch(base);
}