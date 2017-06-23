#include <iostream>

#include "test_common.h"

#include <stdio.h>
#include <stdlib.h>

#include <evpp/libevent.h>
#include <evpp/timestamp.h>
#include <evpp/event_loop_thread.h>

#include <evpp/httpc/request.h>
#include <evpp/httpc/conn.h>
#include <evpp/httpc/response.h>

#include "evpp/http/service.h"
#include "evpp/http/context.h"
#include "evpp/http/http_server.h"

static bool g_stopping = false;

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

static void RequestHandlerHTTPClientRetry(evpp::EventLoop* loop, const evpp::http::ContextPtr& ctx, const evpp::http::HTTPSendResponseCallback& cb) {
    std::stringstream oss;
    oss << "func=" << __FUNCTION__ << " OK"
        << " ip=" << ctx->remote_ip() << "\n"
        << " uri=" << ctx->uri() << "\n"
        << " body=" << ctx->body().ToString() << "\n";
    static std::atomic<int> i(0);
    if (i++ == 0) {
        std::this_thread::sleep_for(std::chrono::seconds(3));
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

    void testRequestHandlerRetry(evpp::EventLoop* loop, int* finished) {
        std::string uri = "/retry";
        std::string url = GetHttpServerURL() + uri;
        auto r = new evpp::httpc::Request(loop, url, "", evpp::Duration(2.0));
        r->set_retry_number(3);
        auto f = [r, finished](const std::shared_ptr<evpp::httpc::Response>& response) {
            std::string result = response->body().ToString();
            H_TEST_ASSERT(!result.empty());
            H_TEST_ASSERT(result.find("uri=/retry") != std::string::npos);
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

    static void TestHTTPClientRetry() {
        evpp::EventLoopThread t;
        t.Start(true);
        int finished = 0;
        testRequestHandlerRetry(t.loop(), &finished);
        testStop(t.loop(), &finished);

        while (true) {
            usleep(10);

            if (finished == 2) {
                break;
            }
        }

        t.Stop(true);
    }

    void testRequestHandlerRetryFailed(evpp::EventLoop* loop, int* finished) {
        std::string uri = "/retry";
        std::string url = GetHttpServerURL() + uri;
        auto r = new evpp::httpc::Request(loop, url, "", evpp::Duration(2.0));
        r->set_retry_number(3);
        auto f = [r, finished](const std::shared_ptr<evpp::httpc::Response>& response) {
            std::string result = response->body().ToString();
            H_TEST_ASSERT(result.empty());
            *finished += 1;
            delete r;
        };

        r->Execute(f);
    }


    static void TestHTTPClientRetryFailed() {
        evpp::EventLoopThread t;
        t.Start(true);
        int finished = 0;
        testRequestHandlerRetryFailed(t.loop(), &finished);

        while (true) {
            usleep(10);

            if (finished == 1) {
                break;
            }
        }

        t.Stop(true);
    }
}

TEST_UNIT(testHTTPClientRetry) {
    for (int i = 0; i < 1; ++i) {
        LOG_INFO << "Running testHTTPClientRetry i=" << i;
        evpp::http::Server ph(i);
        ph.RegisterDefaultHandler(&DefaultRequestHandler);
        ph.RegisterHandler("/retry", &RequestHandlerHTTPClientRetry);
        bool r = ph.Init(g_listening_port) && ph.Start();
        H_TEST_ASSERT(r);
        TestHTTPClientRetry();
        ph.Stop();
        usleep(1000 * 1000); // sleep a while to release the listening address and port
    }
}

TEST_UNIT(testHTTPClientRetryFailed) {
    for (int i = 0; i < 1; ++i) {
        LOG_INFO << "Running testHTTPClientRetry i=" << i;
        TestHTTPClientRetryFailed();
        usleep(1000 * 1000); // sleep a while to release the listening address and port
    }
}
