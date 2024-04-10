#include "test_common.h"

#include <stdio.h>
#include <stdlib.h>

#include <evpp/libevent.h>
#include <evpp/timestamp.h>
#include <evpp/event_loop_thread.h>

#include <evpp/httpc/conn_pool.h>
#include <evpp/httpc/request.h>
#include <evpp/httpc/conn.h>
#include <evpp/httpc/response.h>

namespace {
void http_request_done(struct evhttp_request* req, void* arg) {
    char buf[1024] = {};
    evbuffer_remove(req->input_buffer, &buf, sizeof(buf) - 1);
    printf("%s", buf);
    event_base_loopbreak((struct event_base*)arg);
}
}

TEST_UNIT(evhttpClientSample) {
    struct event_base* base = event_base_new();
    if (base == NULL) {
        return;
    }
#if defined(EVPP_HTTP_CLIENT_SUPPORTS_SSL)
    struct evhttp_connection* conn = evhttp_connection_base_new(base, nullptr, "www.360.cn", 443);
#else
    struct evhttp_connection* conn = evhttp_connection_base_new(base, nullptr, "www.360.cn", 80);
#endif
    if (conn == NULL) {
        return;
    }
    struct evhttp_request* req = evhttp_request_new(http_request_done, base); // will be free by evhttp_connection
    evhttp_add_header(req->output_headers, "Host", "www.360.cn");
    evhttp_make_request(conn, req, EVHTTP_REQ_GET, "/robots.txt");
    evhttp_connection_set_timeout(req->evcon, 600);
    event_base_dispatch(base);
    evhttp_connection_free(conn);
    event_base_free(base);
}

namespace httpc {
static bool responsed = false;
static void HandleHTTPResponse(const std::shared_ptr<evpp::httpc::Response>& r, evpp::EventLoopThread* t) {
    LOG_INFO << "http_code=" << r->http_code() << " [" << r->body().ToString() << "]";
    responsed = true;
    std::string h = r->FindHeader("Connection");
    H_TEST_ASSERT(h == "close");
    delete r->request();
}

void Init() {
    responsed = false;
}

}

TEST_UNIT(testHTTPRequest1) {
    using namespace httpc;
    Init();
    evpp::EventLoopThread t;
    t.Start(true);
#if defined(EVPP_HTTP_CLIENT_SUPPORTS_SSL)
    std::shared_ptr<evpp::httpc::ConnPool> pool(new evpp::httpc::ConnPool("www.360.cn", 443, true, evpp::Duration(2.0)));
#else
    std::shared_ptr<evpp::httpc::ConnPool> pool(new evpp::httpc::ConnPool("www.360.cn", 80, evpp::Duration(2.0)));
#endif
    evpp::httpc::Request* r = new evpp::httpc::Request(pool.get(), t.loop(), "/robots.txt", "");
    LOG_INFO << "Do http request";
    r->Execute(std::bind(&HandleHTTPResponse, std::placeholders::_1, &t));

    while (!responsed) {
        usleep(1);
    }

    pool->Clear();
    usleep(500 * 1000);
    pool.reset();
    t.Stop(true);
    LOG_INFO << "EventLoopThread stopped.";
}

TEST_UNIT(testHTTPRequest2) {
    using namespace httpc;
    Init();
    evpp::EventLoopThread t;
    t.Start(true);
    evpp::httpc::Request* r = new evpp::httpc::Request(t.loop(), "http://www.360.cn/robots.txt?a=1", "", evpp::Duration(2.0));
    LOG_INFO << "Do http request";
    r->Execute(std::bind(&HandleHTTPResponse, std::placeholders::_1, &t));

    while (!responsed) {
        usleep(1);
    }

    t.Stop(true);
    LOG_INFO << "EventLoopThread stopped.";
}


TEST_UNIT(testHTTPRequest3) {
    using namespace httpc;
    Init();
    evpp::EventLoopThread t;
    t.Start(true);
#if defined(EVPP_HTTP_CLIENT_SUPPORTS_SSL)
    std::shared_ptr<evpp::httpc::ConnPool> pool(new evpp::httpc::ConnPool("www.360.cn", 443, true, evpp::Duration(2.0)));
#else
    std::shared_ptr<evpp::httpc::ConnPool> pool(new evpp::httpc::ConnPool("www.360.cn", 80, evpp::Duration(2.0)));
#endif
    evpp::httpc::GetRequest* r = new evpp::httpc::GetRequest(pool.get(), t.loop(), "/robots.txt");
    LOG_INFO << "Do http request";
    r->Execute(std::bind(&HandleHTTPResponse, std::placeholders::_1, &t));

    while (!responsed) {
        usleep(1);
    }

    pool->Clear();
    usleep(500 * 1000);
    pool.reset();
    t.Stop(true);
    LOG_INFO << "EventLoopThread stopped.";
}

TEST_UNIT(testHTTPRequest4) {
    using namespace httpc;
    Init();
    evpp::EventLoopThread t;
    t.Start(true);
    evpp::httpc::PostRequest* r = new evpp::httpc::PostRequest(t.loop(), "http://www.360.cn/robots.txt?a=1", "", evpp::Duration(2.0));
    LOG_INFO << "Do http request";
    r->Execute(std::bind(&HandleHTTPResponse, std::placeholders::_1, &t));

    while (!responsed) {
        usleep(1);
    }

    t.Stop(true);
    LOG_INFO << "EventLoopThread stopped.";
}


namespace hc {
static int responsed = 0;
static int retried = 0;
static void HandleHTTPResponse(const std::shared_ptr<evpp::httpc::Response>& r, evpp::httpc::Request* req, evpp::EventLoopThread* t) {
    LOG_INFO << "http_code=" << r->http_code() << " [" << r->body().ToString() << "]";
    responsed++;
    std::string h = r->FindHeader("Connection");
    H_TEST_ASSERT(h == "close");
    if (retried < 3) {
        retried++;
        req->Execute(std::bind(&hc::HandleHTTPResponse, std::placeholders::_1, req, t));
    } else {
        delete req;
    }
}

void Init() {
    responsed = 0;
}
}

TEST_UNIT(testHTTPRequest5) {
    hc::Init();
    evpp::EventLoopThread t;
    t.Start(true);
    evpp::httpc::PostRequest* r = new evpp::httpc::PostRequest(t.loop(), "http://www.360.cn/robots.txt?a=1", "", evpp::Duration(2.0));
    LOG_INFO << "Do http request";
    r->Execute(std::bind(&hc::HandleHTTPResponse, std::placeholders::_1, r, &t));

    while (hc::responsed != 4) {
        usleep(1);
    }
    H_TEST_ASSERT(hc::retried == 3);
    t.Stop(true);
    LOG_INFO << "EventLoopThread stopped.";
}
