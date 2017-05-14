#include <iostream>

#include <stdio.h>
#include <stdlib.h>

#include <evpp/timestamp.h>
#include <evpp/event_loop_thread.h>
#include <evpp/event_loop.h>

#include <evpp/httpc/request.h>
#include <evpp/httpc/conn.h>
#include <evpp/httpc/response.h>

#include <evpp/http/service.h>
#include <evpp/http/context.h>
#include <evpp/http/http_server.h>

#include "../../examples/winmain-inl.h"

#include "stability.h"

#include "stability_dns.h"
#include "stability_tcp1_client.h"
#include "stability_tcp2_client.h"
#include "stability_tcp3.h"

static bool g_stopping = false;
static void RequestHandler(evpp::EventLoop* loop, const evpp::http::ContextPtr& ctx, const evpp::http::HTTPSendResponseCallback& cb) {
    LOG_INFO << "DefaultRequestHandler loop=" << loop << " ctx.url=" << ctx->original_uri() << " tid=" << std::this_thread::get_id();
    std::stringstream oss;
    oss << "func=" << __FUNCTION__ << " OK"
        << " ip=" << ctx->remote_ip() << "\n"
        << " uri=" << ctx->uri() << "\n"
        << " body=" << ctx->body().ToString() << "\n";
    cb(oss.str());
}

static void DefaultRequestHandler(evpp::EventLoop* loop, const evpp::http::ContextPtr& ctx, const evpp::http::HTTPSendResponseCallback& cb) {
    LOG_INFO << "DefaultRequestHandler loop=" << loop << " ctx.url=" << ctx->original_uri() << " tid=" << std::this_thread::get_id();
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

    static std::string GetHttpServerURL() {
        assert(g_listening_port.size() > 0);
        static int i = 0;
        std::ostringstream oss;
        oss << "http://127.0.0.1:" << g_listening_port[(i++ % g_listening_port.size())];
        return oss.str();
    }

    void testDefaultHandler1(evpp::EventLoop* loop, std::atomic<int>* finished) {
        std::string uri = "/status?a=1";
        std::string url = GetHttpServerURL() + uri;
        auto r = new evpp::httpc::Request(loop, url, "", evpp::Duration(10.0));
        auto f = [r, finished](const std::shared_ptr<evpp::httpc::Response>& response) {
            LOG_INFO << "request=" << r << " response=" << response.get() << " tid=" << std::this_thread::get_id();
            std::string result = response->body().ToString();
            assert(!result.empty());
            assert(result.find("uri=/status") != std::string::npos);
            assert(result.find("uri=/status?a=1") == std::string::npos);
            assert(result.find("func=DefaultRequestHandler") != std::string::npos);
            finished->fetch_add(1);
            delete r;
        };

        r->Execute(f);
    }

    void testDefaultHandler2(evpp::EventLoop* loop, std::atomic<int>* finished) {
        std::string uri = "/status";
        std::string body = "The http request body.";
        std::string url = GetHttpServerURL() + uri;
        auto r = new evpp::httpc::Request(loop, url, body, evpp::Duration(10.0));
        auto f = [body, r, finished](const std::shared_ptr<evpp::httpc::Response>& response) {
            LOG_INFO << "request=" << r << " response=" << response.get() << " tid=" << std::this_thread::get_id();
            std::string result = response->body().ToString();
            assert(!result.empty());
            assert(result.find("uri=/status") != std::string::npos);
            assert(result.find("func=DefaultRequestHandler") != std::string::npos);
            assert(result.find(body.c_str()) != std::string::npos);
            finished->fetch_add(1);
            delete r;
        };

        r->Execute(f);
    }

    void testDefaultHandler3(evpp::EventLoop* loop, std::atomic<int>* finished) {
        std::string uri = "/status/method/method2/xx";
        std::string url = GetHttpServerURL() + uri;
        auto r = new evpp::httpc::Request(loop, url, "", evpp::Duration(10.0));
        auto f = [r, finished](const std::shared_ptr<evpp::httpc::Response>& response) {
            LOG_INFO << "request=" << r << " response=" << response.get() << " tid=" << std::this_thread::get_id();
            std::string result = response->body().ToString();
            assert(!result.empty());
            assert(result.find("uri=/status/method/method2/xx") != std::string::npos);
            assert(result.find("func=DefaultRequestHandler") != std::string::npos);
            finished->fetch_add(1);
            delete r;
        };

        r->Execute(f);
    }

    void testPushBootHandler(evpp::EventLoop* loop, std::atomic<int>* finished) {
        std::string uri = "/push/boot";
        std::string url = GetHttpServerURL() + uri;
        auto r = new evpp::httpc::Request(loop, url, "", evpp::Duration(10.0));
        auto f = [r, finished](const std::shared_ptr<evpp::httpc::Response>& response) {
            LOG_INFO << "request=" << r << " response=" << response.get() << " tid=" << std::this_thread::get_id();
            std::string result = response->body().ToString();
            assert(!result.empty());
            assert(result.find("uri=/push/boot") != std::string::npos);
            assert(result.find("func=RequestHandler") != std::string::npos);
            finished->fetch_add(1);
            delete r;
        };

        r->Execute(f);
    }

    void testStop(evpp::EventLoop* loop, std::atomic<int>* finished) {
        std::string uri = "/mod/stop";
        std::string url = GetHttpServerURL() + uri;
        auto r = new evpp::httpc::Request(loop, url, "", evpp::Duration(10.0));
        auto f = [r, finished](const std::shared_ptr<evpp::httpc::Response>& response) {
            LOG_INFO << "request=" << r << " response=" << response.get() << " tid=" << std::this_thread::get_id();
            std::string result = response->body().ToString();
            assert(!result.empty());
            assert(result.find("uri=/mod/stop") != std::string::npos);
            assert(result.find("func=DefaultRequestHandler") != std::string::npos);
            finished->fetch_add(1);
            delete r;
        };

        r->Execute(f);
    }

    static void TestAll() {
        LOG_INFO << "TestAll start";
        evpp::EventLoopThread t;
        t.Start(true);
        std::atomic<int> finished(0);
        testDefaultHandler1(t.loop(), &finished);
        testDefaultHandler2(t.loop(), &finished);
        testDefaultHandler3(t.loop(), &finished);
        testPushBootHandler(t.loop(), &finished);
        testStop(t.loop(), &finished);

        while (true) {
            usleep(10);

            if (finished.load() == 5) {
                break;
            }
        }

        t.Stop(true);
        LOG_INFO << "TestAll end";
    }
}

void TestHTTPServer() {
    for (int i = 0; i < 40; ++i) {
        LOG_INFO << "Running TestHTTPServer i=" << i;
        evpp::http::Server ph(i);
        ph.RegisterDefaultHandler(&DefaultRequestHandler);
        ph.RegisterHandler("/push/boot", &RequestHandler);
        bool r = ph.Init(g_listening_port) && ph.Start();
        assert(r);
        (void)r;
        TestAll();
        ph.Stop();
        //usleep(1000 * 1000); // sleep a while to release the listening address and port
    }
}


int main(int argc, char* argv[]) {
    int port = 8080;
    if (argc > 1) {
        if (std::string("-h") == argv[1] ||
                std::string("--h") == argv[1] ||
                std::string("-help") == argv[1] ||
                std::string("--help") == argv[1]) {
            std::cout << "usage : " << argv[0] << " <listen_port1> <listen_port2> ...\n";
            std::cout << " e.g. : " << argv[0] << " 8080\n";
            std::cout << " e.g. : " << argv[0] << " 8080 8081\n";
            std::cout << " e.g. : " << argv[0] << " 8080 8081 8082\n";
            return 0;
        }
    }

    if (argc == 1) {
        g_listening_port.push_back (port);
    } else {
        for (int i = 1; i < argc; i++) {
            port = std::atoi(argv[i]);
            g_listening_port.push_back(port);
        }
    }

    // We are running forever
    // If the program stops at somewhere there must be a bug to be fixed.
    for (size_t i = 0;;i++) {
        LOG_WARN << "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx Running test loop TestTCPServer1 " << i;
        TestTCPServer1();
        LOG_WARN << "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx Running test loop TestHTTPServer " << i;
        TestHTTPServer();
        LOG_WARN << "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx Running test loop TestTCPClientReconnect " << i;
        TestTCPClientReconnect();
        LOG_WARN << "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx Running test loop TestTCPClientConnectFailed " << i;
        TestTCPClientConnectFailed();
        LOG_WARN << "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx Running test loop TestTCPClientDisconnectImmediately " << i;
        TestTCPClientDisconnectImmediately();
        LOG_WARN << "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx Running test loop TestTCPClientDisconnectAndDestruct " << i;
        TestTCPClientDisconnectAndDestruct();
        LOG_WARN << "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx Running test loop TestTCPClientConnectLocalhost " << i;
        TestTCPClientConnectLocalhost();
        LOG_WARN << "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx Running test loop TestTCPServerSilenceShutdown1 " << i;
        TestTCPServerSilenceShutdown1();
        LOG_WARN << "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx Running test loop TestTCPServerSilenceShutdown2 " << i;
        TestTCPServerSilenceShutdown2();
        LOG_WARN << "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx Running test loop TestDNSResolver " << i;
        TestDNSResolver();
    }
    return 0;
}
