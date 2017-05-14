#include <iostream>

#include "test_common.h"

#include <stdio.h>
#include <stdlib.h>

#include <evpp/libevent_headers.h>
#include <evpp/timestamp.h>
#include <evpp/event_loop_thread.h>

#include <evpp/httpc/request.h>
#include <evpp/httpc/conn.h>
#include <evpp/httpc/response.h>

#include "evpp/http/service.h"
#include "evpp/http/context.h"
#include "evpp/http/http_server.h"

static bool g_stopping = false;
static void RequestHandler(evpp::EventLoop* loop, const evpp::http::ContextPtr& ctx, const evpp::http::HTTPSendResponseCallback& cb) {
    std::stringstream oss;
    oss << "func=" << __FUNCTION__ << " OK"
        << " ip=" << ctx->remote_ip() << "\n"
        << " uri=" << ctx->uri() << "\n"
        << " body=" << ctx->body().ToString() << "\n";
    cb(oss.str());
}

static void RequestHandler201(evpp::EventLoop* loop, const evpp::http::ContextPtr& ctx, const evpp::http::HTTPSendResponseCallback& cb) {
    std::stringstream oss;
    oss << "func=" << __FUNCTION__ << " OK"
        << " ip=" << ctx->remote_ip() << "\n"
        << " uri=" << ctx->uri() << "\n"
        << " body=" << ctx->body().ToString() << "\n";
    ctx->set_response_http_code(201);
    cb(oss.str());
}

static void RequestHandler909(evpp::EventLoop* loop, const evpp::http::ContextPtr& ctx, const evpp::http::HTTPSendResponseCallback& cb) {
    std::stringstream oss;
    oss << "func=" << __FUNCTION__ << " OK"
        << " ip=" << ctx->remote_ip() << "\n"
        << " uri=" << ctx->uri() << "\n"
        << " body=" << ctx->body().ToString() << "\n";
    ctx->set_response_http_code(909);
    cb(oss.str());
}

static void DefaultRequestHandler(evpp::EventLoop* loop, const evpp::http::ContextPtr& ctx, const evpp::http::HTTPSendResponseCallback& cb) {
    //std::cout << __func__ << " called ...\n";
    std::stringstream oss;
    oss << "func=" << __FUNCTION__ << "\n"
        << " ip=" << ctx->remote_ip() << "\n"
        << " uri=" << ctx->uri() << "\n"
        << " body=" << ctx->body().ToString() << "\n";

    if (ctx->uri().find("stop") != std::string::npos) {
        g_stopping = true;
    }

    cb(oss.str());
}

namespace {

static std::vector<int> g_listening_port = { 49000, 49001 };

static std::string GetHttpServerURL() {
    static int i = 0;
    std::ostringstream oss;
    oss << "http://127.0.0.1:" << g_listening_port[(i++ % g_listening_port.size())];
    return oss.str();
}

void testDefaultHandler1(evpp::EventLoop* loop, int* finished) {
    std::string uri = "/status?a=1";
    std::string url = GetHttpServerURL() + uri;
    auto r = new evpp::httpc::Request(loop, url, "", evpp::Duration(10.0));
    auto f = [r, finished](const std::shared_ptr<evpp::httpc::Response>& response) {
        std::string result = response->body().ToString();
        H_TEST_ASSERT(!result.empty());
        H_TEST_ASSERT(result.find("uri=/status") != std::string::npos);
        H_TEST_ASSERT(result.find("uri=/status?a=1") == std::string::npos);
        H_TEST_ASSERT(result.find("func=DefaultRequestHandler") != std::string::npos);
        *finished += 1;
        delete r;
    };

    r->Execute(f);
}

void testDefaultHandler2(evpp::EventLoop* loop, int* finished) {
    std::string uri = "/status";
    std::string body = "The http request body.";
    std::string url = GetHttpServerURL() + uri;
    auto r = new evpp::httpc::Request(loop, url, body, evpp::Duration(10.0));
    auto f = [body, r, finished](const std::shared_ptr<evpp::httpc::Response>& response) {
        std::string result = response->body().ToString();
        H_TEST_ASSERT(!result.empty());
        H_TEST_ASSERT(result.find("uri=/status") != std::string::npos);
        H_TEST_ASSERT(result.find("func=DefaultRequestHandler") != std::string::npos);
        H_TEST_ASSERT(result.find(body.c_str()) != std::string::npos);
        *finished += 1;
        delete r;
    };

    r->Execute(f);
}

void testDefaultHandler3(evpp::EventLoop* loop, int* finished) {
    std::string uri = "/status/method/method2/xx";
    std::string url = GetHttpServerURL() + uri;
    auto r = new evpp::httpc::Request(loop, url, "", evpp::Duration(10.0));
    auto f = [r, finished](const std::shared_ptr<evpp::httpc::Response>& response) {
        std::string result = response->body().ToString();
        H_TEST_ASSERT(!result.empty());
        H_TEST_ASSERT(result.find("uri=/status/method/method2/xx") != std::string::npos);
        H_TEST_ASSERT(result.find("func=DefaultRequestHandler") != std::string::npos);
        *finished += 1;
        delete r;
    };

    r->Execute(f);
}

void testPushBootHandler(evpp::EventLoop* loop, int* finished) {
    std::string uri = "/push/boot";
    std::string url = GetHttpServerURL() + uri;
    auto r = new evpp::httpc::Request(loop, url, "", evpp::Duration(10.0));
    auto f = [r, finished](const std::shared_ptr<evpp::httpc::Response>& response) {
        std::string result = response->body().ToString();
        H_TEST_ASSERT(!result.empty());
        H_TEST_ASSERT(result.find("uri=/push/boot") != std::string::npos);
        H_TEST_ASSERT(result.find("func=RequestHandler") != std::string::npos);
        *finished += 1;
        delete r;
    };

    r->Execute(f);
}

void testRequestHandler201(evpp::EventLoop* loop, int* finished) {
    std::string uri = "/201";
    std::string url = GetHttpServerURL() + uri;
    auto r = new evpp::httpc::Request(loop, url, "", evpp::Duration(10.0));
    auto f = [r, finished](const std::shared_ptr<evpp::httpc::Response>& response) {
        std::string result = response->body().ToString();
        H_TEST_ASSERT(!result.empty());
        H_TEST_ASSERT(response->http_code() == 201);
        H_TEST_ASSERT(result.find("uri=/201") != std::string::npos);
        H_TEST_ASSERT(result.find("func=RequestHandler201") != std::string::npos);
        *finished += 1;
        delete r;
    };

    r->Execute(f);
}

void testRequestHandler909(evpp::EventLoop* loop, int* finished) {
    std::string uri = "/909";
    std::string url = GetHttpServerURL() + uri;
    auto r = new evpp::httpc::Request(loop, url, "", evpp::Duration(10.0));
    auto f = [r, finished](const std::shared_ptr<evpp::httpc::Response>& response) {
        std::string result = response->body().ToString();
        H_TEST_ASSERT(!result.empty());
        H_TEST_ASSERT(response->http_code() == 909);
        H_TEST_ASSERT(result.find("uri=/909") != std::string::npos);
        H_TEST_ASSERT(result.find("func=RequestHandler909") != std::string::npos);
        *finished += 1;
        delete r;
    };

    r->Execute(f);
}

void testStop(evpp::EventLoop* loop, int* finished) {
    std::string uri = "/mod/stop";
    std::string url = GetHttpServerURL() + uri;
    auto r = new evpp::httpc::Request(loop, url, "", evpp::Duration(10.0));
    auto f = [r, finished](const std::shared_ptr<evpp::httpc::Response>& response) {
        std::string result = response->body().ToString();
        H_TEST_ASSERT(!result.empty());
        H_TEST_ASSERT(result.find("uri=/mod/stop") != std::string::npos);
        H_TEST_ASSERT(result.find("func=DefaultRequestHandler") != std::string::npos);
        *finished += 1;
        delete r;
    };

    r->Execute(f);
}

static void TestAll() {
    evpp::EventLoopThread t;
    t.Start(true);
    int finished = 0;
    testDefaultHandler1(t.loop(), &finished);
    testDefaultHandler2(t.loop(), &finished);
    testDefaultHandler3(t.loop(), &finished);
    testPushBootHandler(t.loop(), &finished);
    testRequestHandler201(t.loop(), &finished);
    testRequestHandler909(t.loop(), &finished);
    testStop(t.loop(), &finished);

    while (true) {
        usleep(10);

        if (finished == 7) {
            break;
        }
    }

    t.Stop(true);
}
}


TEST_UNIT(testHTTPServer) {
    for (int i = 0; i < 5; ++i) {
        LOG_INFO << "Running testHTTPServer i=" << i;
        evpp::http::Server ph(i);
        ph.RegisterDefaultHandler(&DefaultRequestHandler);
        ph.RegisterHandler("/push/boot", &RequestHandler);
        ph.RegisterHandler("/201", &RequestHandler201);
        ph.RegisterHandler("/909", &RequestHandler909);
        bool r = ph.Init(g_listening_port) && ph.Start();
        H_TEST_ASSERT(r);
        TestAll();
        ph.Stop();
        usleep(1000 * 1000); // sleep a while to release the listening address and port
    }
}

TEST_UNIT(testFindClientIP) {
    struct TestCase {
        std::string uri;
        std::string ip;
    } cases[] = {
        {"/abc?clientip=", ""},
        {"/abc?clientip=123.1.1.9", "123.1.1.9"},
        {"/abc?clientip=123.1.1.9&a=b", "123.1.1.9"},
        {"/abc?c=d&clientip=123.1.1.9&a=b", "123.1.1.9"},
        {"/abc?c=d&xx=123.1.1.9&a=b", ""},
    };

    for (size_t i = 0; i < H_ARRAYSIZE(cases); i++) {
        std::string ip = evpp::http::Context::FindClientIP(cases[i].uri.data());
        H_TEST_ASSERT(ip == cases[i].ip);
    }
}
