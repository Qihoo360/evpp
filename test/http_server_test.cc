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

#include "evpp/https/embedded_http_server.h"
#include "evpp/https/standalone_http_server.h"

static bool g_stopping = false;
static void RequestHandler(const evpp::https::HTTPContextPtr& ctx, const evpp::https::HTTPSendResponseCallback& cb) {
    std::stringstream oss;
    oss << "func=" << __FUNCTION__ << " OK"
        << " ip=" << ctx->remote_ip << "\n"
        << " uri=" << ctx->uri << "\n"
        << " body=" << ctx->body.ToString() << "\n";
    cb(oss.str());
}

static void DefaultRequestHandler(const evpp::https::HTTPContextPtr& ctx, const evpp::https::HTTPSendResponseCallback& cb) {
    //std::cout << __func__ << " called ...\n";
    std::stringstream oss;
    oss << "func=" << __FUNCTION__ << "\n"
        << " ip=" << ctx->remote_ip << "\n"
        << " uri=" << ctx->uri << "\n"
        << " body=" << ctx->body.ToString() << "\n";

    if (ctx->params) {
        evpp::https::HTTPParameterMap::const_iterator it(ctx->params->begin());
        evpp::https::HTTPParameterMap::const_iterator ite(ctx->params->end());
        for (; it != ite; ++it) {
            oss << "key=" << it->first << " value=[" << it->second << "]\n";
        }
    }

    if (ctx->uri.find("stop") != std::string::npos) {
        g_stopping = true;
    }
    cb(oss.str());
}

namespace {


    static int g_listening_port = 49000;

    static std::string GetHttpServerURL() {
        std::ostringstream oss;
        oss << "http://127.0.0.1:" << g_listening_port;
        return oss.str();
    }

    void testDefaultHandler1(evpp::EventLoop* loop) {
        std::string uri = "/status?a=1";
        std::string url = GetHttpServerURL() + uri;
        auto f = [](const std::shared_ptr<evpp::httpc::Response>& response, evpp::httpc::Request* request) {
            std::string result = response->body().ToString();
            H_TEST_ASSERT(!result.empty());
            H_TEST_ASSERT(result.find("uri=/status") != std::string::npos);
            H_TEST_ASSERT(result.find("uri=/status?a=1") == std::string::npos);
            H_TEST_ASSERT(result.find("func=DefaultRequestHandler") != std::string::npos);
            delete request;
            g_stopping = true;
        };

        auto r = new evpp::httpc::Request(loop, url, "", evpp::Duration(1.0));
        r->Execute(std::bind(f, std::placeholders::_1, r));
    }

    void testDefaultHandler2(evpp::EventLoop* loop) {
        std::string uri = "/status";
        std::string body = "The http request body.";
        std::string url = GetHttpServerURL() + uri;
        auto f = [body](const std::shared_ptr<evpp::httpc::Response>& response, evpp::httpc::Request* request) {
            std::string result = response->body().ToString();
            H_TEST_ASSERT(!result.empty());
            H_TEST_ASSERT(result.find("uri=/status") != std::string::npos);
            H_TEST_ASSERT(result.find("func=DefaultRequestHandler") != std::string::npos);
            H_TEST_ASSERT(result.find(body.c_str()) != std::string::npos);
            delete request;
            g_stopping = true;
        };

        auto r = new evpp::httpc::Request(loop, url, body, evpp::Duration(1.0));
        r->Execute(std::bind(f, std::placeholders::_1, r));
    }

    void testDefaultHandler3(evpp::EventLoop* loop) {
        std::string uri = "/status/method/method2/xx";
        std::string url = GetHttpServerURL() + uri;
        auto f = [](const std::shared_ptr<evpp::httpc::Response>& response, evpp::httpc::Request* request) {
            std::string result = response->body().ToString();
            H_TEST_ASSERT(!result.empty());
            H_TEST_ASSERT(result.find("uri=/status/method/method2/xx") != std::string::npos);
            H_TEST_ASSERT(result.find("func=DefaultRequestHandler") != std::string::npos);
            delete request;
            g_stopping = true;
        };

        auto r = new evpp::httpc::Request(loop, url, "", evpp::Duration(1.0));
        r->Execute(std::bind(f, std::placeholders::_1, r));
    }

    void testPushBootHandler(evpp::EventLoop* loop) {
        std::string uri = "/push/boot";
        std::string url = GetHttpServerURL() + uri;
        auto f = [](const std::shared_ptr<evpp::httpc::Response>& response, evpp::httpc::Request* request) {
            std::string result = response->body().ToString();
            H_TEST_ASSERT(!result.empty());
            H_TEST_ASSERT(result.find("uri=/push/boot") != std::string::npos);
            H_TEST_ASSERT(result.find("func=RequestHandler") != std::string::npos);
            delete request;
            g_stopping = true;
        };

        auto r = new evpp::httpc::Request(loop, url, "", evpp::Duration(1.0));
        r->Execute(std::bind(f, std::placeholders::_1, r));
    }

    void testStop(evpp::EventLoop* loop) {
        std::string uri = "/mod/stop";
        std::string url = GetHttpServerURL() + uri;
        auto f = [](const std::shared_ptr<evpp::httpc::Response>& response, evpp::httpc::Request* request) {
            std::string result = response->body().ToString();
            H_TEST_ASSERT(!result.empty());
            H_TEST_ASSERT(result.find("uri=/mod/stop") != std::string::npos);
            H_TEST_ASSERT(result.find("func=DefaultRequestHandler") != std::string::npos);
            delete request;
            g_stopping = true;
        };

        auto r = new evpp::httpc::Request(loop, url, "", evpp::Duration(1.0));
        r->Execute(std::bind(f, std::placeholders::_1, r));
    }

    static void TestAll() {
        evpp::EventLoopThread t;
        t.Start(true);
        testDefaultHandler1(t.event_loop());
        testDefaultHandler2(t.event_loop());
        testDefaultHandler3(t.event_loop());
        testPushBootHandler(t.event_loop());
        testStop(t.event_loop());

        while (true) {
            usleep(10);
            if (g_stopping) {
                break;
            }
        }
        t.Stop(true);
    }
}


TEST_UNIT(testHTTPServer) {
    for (int i = 0; i < 3; ++i) {
        g_stopping = true;
        evpp::https::StandaloneHTTPServer ph(i);
        ph.RegisterDefaultEvent(&DefaultRequestHandler);
        ph.RegisterEvent("/push/boot", &RequestHandler);
        bool r = ph.Start(g_listening_port, true);
        H_TEST_ASSERT(r);
        TestAll();
        ph.Stop(true);
    }
}
