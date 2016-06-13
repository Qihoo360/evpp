
#include "evpp/exp.h"
#include "test_common.h"

#include "evpp/libevent_headers.h"
#include "evpp/libevent_watcher.h"
#include "evpp/event_loop.h"
#include "evpp/event_loop_thread.h"
#include "evpp/tcp_server.h"
#include "evpp/buffer.h"
#include "evpp/tcp_conn.h"
#include "evpp/tcp_client.h"

#include <thread>

namespace {
    static bool connected = false;
    static bool message_recved = false;
    static int count = 0;
    const static std::string addr = "127.0.0.1:9099";
    static void OnMessage(const evpp::TCPConnPtr& conn,
                   evpp::Buffer* msg,
                   evpp::Timestamp ts) {
        message_recved = true;
        count = 1;
    }

    static void StopTCPServer(evpp::TCPServer* t) {
        t->Stop();
    }

    void OnClientConnection(const evpp::TCPConnPtr& conn) {
        if (conn->IsConnected()) {
            conn->Send("hello");
            connected = true;
        } else {
            LOG_INFO << "Disconnected from " << conn->remote_addr();
        }
    }

    xstd::shared_ptr<evpp::TCPClient> StartTCPClient(evpp::EventLoop* loop) {
        xstd::shared_ptr<evpp::TCPClient> client(new evpp::TCPClient(loop, addr, "TCPPingPongClient"));
        client->SetConnectionCallback(&OnClientConnection);
        client->Connect();
        loop->RunAfter(evpp::Duration(1.0), xstd::bind(&evpp::TCPClient::Disconnect, client));
        loop->RunAfter(evpp::Duration(1.1), xstd::bind(&evpp::EventLoop::Stop, loop));
        return client;
    }
}


TEST_UNIT(testATCPServer) {
    evpp::EventLoopThread t;
    t.Start();
    evpp::EventLoop loop;
    evpp::TCPServer tsrv(&loop, addr, "tcp_server", 2);
    tsrv.SetMesageHandler(&OnMessage);
    tsrv.Start();
    loop.RunAfter(evpp::Duration(0.8), xstd::bind(&StopTCPServer, &tsrv));
    loop.RunAfter(evpp::Duration(0.9), xstd::bind(&evpp::EventLoop::Stop, &loop));
    xstd::shared_ptr<evpp::TCPClient> client = StartTCPClient(t.event_loop());
    loop.Run();
    t.Stop(true);
    H_TEST_ASSERT(connected);
    H_TEST_ASSERT(count == 1);
    H_TEST_ASSERT(message_recved);
}


