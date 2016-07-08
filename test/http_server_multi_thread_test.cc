
#include "osl/include/exp.h"
#include "osl/include/thread.h"
#include "osl/include/test_common.h"
#include "osl/include/ini_parser.h"
#include "osl/include/logger.h"

#include <iostream>

#include "evqing/include/embedded_http_server.h"
#include "evqing/include/standalone_http_server.h"

#include "net/include/exp.h"
#include "net/include/curl_work.h"

static std::set<osl::Thread::ThreadHandle> g_working_tids;

static void OnWorkingThread()
{
    static osl::MutexLock mutex;
    osl::MutexLockGuard g(mutex);
    g_working_tids.insert(osl::Thread::getCurrentThreadID());
}

static bool g_stopping = false;
static void RequestHandler(const evqing::HTTPContextPtr& ctx, const evqing::HTTPSendResponseCallback& cb)
{
    //std::cout << __func__ << " called ...\n";
    std::stringstream oss;
    oss << "func=" << __FUNCTION__ << "\n"
        << " ip=" << ctx->remote_ip << "\n"
        << " uri=" << ctx->uri << "\n";

    cb(oss.str());

    OnWorkingThread();
}

static void DefaultRequestHandler(const evqing::HTTPContextPtr& ctx, const evqing::HTTPSendResponseCallback& cb)
{
    //std::cout << __func__ << " called ...\n";
    std::stringstream oss;
    oss << "func=" << __FUNCTION__ << "\n"
        << " ip=" << ctx->remote_ip << "\n"
        << " uri=" << ctx->uri << "\n";

    evqing::HTTPParameterMap::const_iterator it(ctx->params->begin());
    evqing::HTTPParameterMap::const_iterator ite(ctx->params->end());
    for (; it != ite; ++it) {
        oss << "key=" << it->first << " value=[" << it->second << "]\n";
    }

    std::string httpHeaderKeys[] = { "Accept", "Content-Length", "Host" };
    for (size_t i = 0; i < H_ARRAY_SIZE(httpHeaderKeys); i++)
    {
        if (ctx->params->find(httpHeaderKeys[i]) == ite)
        {
            assert(false);
            LOG_FATAL << "Cannot find request HTTP header : " << httpHeaderKeys[i];
        }
    }

    if (ctx->uri.find("stop") != std::string::npos)
    {
        g_stopping = true;
    }

    cb(oss.str());

    OnWorkingThread();
}

namespace
{
    static int g_listening_port = 49000;

    static std::string GetHttpServerURL() {
        std::ostringstream oss;
        oss << "http://127.0.0.1:" << g_listening_port;
        return oss.str();
    }

    void testDefaultHandler1()
    {
        std::string uri = "/status";
        std::string url = GetHttpServerURL() + uri;
        std::string result = net::CURLWork::post(url, std::string("http-body") + __func__ , NULL);
        H_TEST_ASSERT(!result.empty());
        osl::INIParser ini;
        H_TEST_ASSERT(ini.parse(result.data(), result.size()));
        bool found = false;
        H_TEST_ASSERT(ini.get("uri", found) == uri);
        H_TEST_ASSERT(ini.get("func", found) == "DefaultRequestHandler");
    }

    void testDefaultHandler2()
    {
        std::string uri = "/status";
        std::string url = GetHttpServerURL() + uri;
        std::string result = net::CURLWork::post(url, "http-body", NULL);
        H_TEST_ASSERT(!result.empty());
        osl::INIParser ini;
        H_TEST_ASSERT(ini.parse(result.data(), result.size()));
        bool found = false;
        H_TEST_ASSERT(ini.get("uri", found) == uri);
        H_TEST_ASSERT(ini.get("func", found) == "DefaultRequestHandler");
    }

    void testDefaultHandler3()
    {
        std::string uri = "/status/method/method2/xx?x=1";
        std::string url = GetHttpServerURL() + uri;
        std::string result = net::CURLWork::post(url, "http-body", NULL);
        H_TEST_ASSERT(!result.empty());
        osl::INIParser ini;
        H_TEST_ASSERT(ini.parse(result.data(), result.size()));
        bool found = false;
        H_TEST_ASSERT(ini.get("uri", found) == "/status/method/method2/xx");
        H_TEST_ASSERT(ini.get("func", found) == "DefaultRequestHandler");
    }

    void testPushBootHandler()
    {
        std::string uri = "/push/boot";
        std::string url = GetHttpServerURL() + uri;
        std::string result = net::CURLWork::post(url, "http-body", NULL);
        H_TEST_ASSERT(!result.empty());
        osl::INIParser ini;
        H_TEST_ASSERT(ini.parse(result.data(), result.size()));
        bool found = false;
        H_TEST_ASSERT(ini.get("uri", found) == uri);
        H_TEST_ASSERT(ini.get("func", found) == "RequestHandler");
    }

    void testStop()
    {
        std::string uri = "/mod/stop";
        std::string url = GetHttpServerURL() + uri;
        std::string result = net::CURLWork::post(url, "http-body", NULL);
        osl::INIParser ini;
        H_TEST_ASSERT(ini.parse(result.data(), result.size()));
        bool found = false;
        H_TEST_ASSERT(ini.get("uri", found) == uri);
        H_TEST_ASSERT(ini.get("func", found) == "DefaultRequestHandler");
    }

    static void TestAll(int thread_num) {
        for (int i = 0; i < thread_num; i++)
        {
            testDefaultHandler1();
            testDefaultHandler2();
            testDefaultHandler3();
            testPushBootHandler();
        }
        testStop();

        while (true) {
            osl::Process::msleep(10);
            if (g_stopping) {
                break;
            }
        }
    }
}


TEST_UNIT(test_evqing_HTTPServer2)
{
    g_working_tids.clear();
    int thread_num = 24;
    evqing::StandaloneHTTPServer ph(thread_num);
    ph.http_service()->set_parse_parameters(true);
    ph.http_service()->AddRequestHeaderKeyForParsing("Content-Length");
    ph.http_service()->AddRequestHeaderKeyForParsing("User-Agent");
    ph.http_service()->AddRequestHeaderKeyForParsing("Host");
    ph.http_service()->AddRequestHeaderKeyForParsing("Accept");
    ph.RegisterDefaultEvent(&DefaultRequestHandler);
    ph.RegisterEvent("/push/boot", &RequestHandler);
    bool r = ph.Start(g_listening_port, true);
    H_TEST_ASSERT(r);
    TestAll(thread_num);
    ph.Stop(true);
    H_TEST_ASSERT(g_working_tids.size() == thread_num);
}
// 
// TEST_UNIT(test_evqing_HTTPServer3)
// {
//     g_working_tids.clear();
//     int thread_num = 24;
//     evqing::StandaloneHTTPServer ph(thread_num);
//     ph.http_service()->set_send_response_in_main_thread(true);
//     ph.RegisterDefaultEvent(&DefaultRequestHandler);
//     ph.RegisterEvent("/push/boot", &RequestHandler);
//     bool r = ph.Start(g_listening_port, true);
//     H_TEST_ASSERT(r);
//     TestAll(thread_num);
//     ph.Stop(true);
//     H_TEST_ASSERT(g_working_tids.size() == thread_num);
// }

