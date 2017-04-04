#include "test_common.h"

#include <evpp/udp/sync_udp_client.h>
#include <evpp/udp/udp_server.h>

namespace {
static int g_count = 0;
static bool g_exit = false;
static uint64_t g_timeout_ms = 1000;
static void OnMessage(evpp::udp::Server* udpsrv, evpp::EventLoop* loop, const evpp::udp::MessagePtr msg) {
    g_count++;
    evpp::udp::SendMessage(msg);
    usleep(100);
    if (memcmp(msg->data(), "stop", msg->size()) == 0) {
        g_exit = true;
    }
}

static void Init() {
    g_count = 0;
    g_exit = false;
}
}

TEST_UNIT(testUDPServer) {
    LOG_TRACE << __func__;
    Init();
    std::vector<int> ports(2, 0);
    ports[0] = 15368;
    ports[1] = 15369;
    evpp::udp::Server* udpsrv = new evpp::udp::Server;
    udpsrv->SetMessageHandler(std::bind(&OnMessage, udpsrv, std::placeholders::_1, std::placeholders::_2));
    H_TEST_ASSERT(udpsrv->Init(ports) && udpsrv->Start());
    usleep(100);//wait udpsrv started
    LOG_TRACE << "udpserver started.";

    int loop = 10;
    for (int i = 0; i < loop; ++i) {
        std::string req = "data xxx";
        std::string resp = evpp::udp::sync::Client::DoRequest("127.0.0.1", ports[0], req, g_timeout_ms);
        H_TEST_ASSERT(req == resp);
        resp = evpp::udp::sync::Client::DoRequest("127.0.0.1", ports[1], req, g_timeout_ms);
        H_TEST_ASSERT(req == resp);
    }

    H_TEST_ASSERT(g_count == 2 * loop);
    evpp::udp::sync::Client::DoRequest("127.0.0.1", ports[0], "stop", g_timeout_ms);
    H_TEST_ASSERT(g_count == 2 * loop + 1);
    evpp::udp::sync::Client::DoRequest("127.0.0.1", ports[1], "stop", g_timeout_ms);
    H_TEST_ASSERT(g_count == 2 * loop + 2);

    while (!g_exit) {
        usleep(1);
    }

    H_TEST_ASSERT(g_exit);
    udpsrv->Stop(true);
    H_TEST_ASSERT(udpsrv->IsStopped());
    delete udpsrv;
}


TEST_UNIT(testUDPServerGraceStop) {
    LOG_TRACE << __func__;
    Init();
    int port = 53669;
    evpp::udp::Server* udpsrv = new evpp::udp::Server;
    udpsrv->SetMessageHandler(std::bind(&OnMessage, udpsrv, std::placeholders::_1, std::placeholders::_2));
    H_TEST_ASSERT(udpsrv->Init(port) && udpsrv->Start());
    usleep(100);//wait udpsrv started

    int loop = 10;
    for (int i = 0; i < loop; ++i) {
        std::string req = "data xxx";
        std::string resp = evpp::udp::sync::Client::DoRequest("127.0.0.1", port, req, g_timeout_ms);
        H_TEST_ASSERT(req == resp);
    }

    H_TEST_ASSERT(g_count == loop);
    evpp::udp::sync::Client::DoRequest("127.0.0.1", port, "stop", g_timeout_ms);
    H_TEST_ASSERT(g_count == loop + 1);

    while (!g_exit) {
        usleep(1);
    }

    H_TEST_ASSERT(g_exit);
    udpsrv->Stop(true);
    H_TEST_ASSERT(udpsrv->IsStopped());
    delete udpsrv;
}