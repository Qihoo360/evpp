#include "test_common.h"

#include <evpp/libevent.h>
#include <evpp/event_watcher.h>
#include <evpp/event_loop.h>
#include <evpp/event_loop_thread.h>
#include <evpp/tcp_server.h>
#include <evpp/buffer.h>
#include <evpp/tcp_conn.h>
#include <evpp/tcp_client.h>


#include <thread>

namespace {
static std::shared_ptr<evpp::TCPServer> tsrv;
static std::atomic<int> connected_count(0);
static std::atomic<int> message_recved_count(0);
const static std::string addr = "127.0.0.1:19099";

void OnClientConnection(const evpp::TCPConnPtr& conn) {
    if (conn->IsConnected()) {
        conn->Send("hello");
        LOG_INFO << "Send a message to server when connected.";
        connected_count++;
    } else {
        LOG_INFO << "Disconnected from " << conn->remote_addr();
    }
}

evpp::TCPClient* StartTCPClient(evpp::EventLoop* loop) {
    evpp::TCPClient* client(new evpp::TCPClient(loop, addr, "TCPPingPongClient"));
    client->set_reconnect_interval(evpp::Duration(0.1));
    client->SetConnectionCallback(&OnClientConnection);
    client->Connect();
    return client;
}

}


TEST_UNIT(testTCPClientReconnect) {
    std::unique_ptr<evpp::EventLoopThread> tcp_client_thread(new evpp::EventLoopThread);
    tcp_client_thread->set_name("TCPClientThread");
    tcp_client_thread->Start(true);
    std::unique_ptr<evpp::EventLoopThread> tcp_server_thread(new evpp::EventLoopThread);
    tcp_server_thread->set_name("TCPServerThread");
    tcp_server_thread->Start(true);
    evpp::TCPClient* client = StartTCPClient(tcp_client_thread->loop());

    int test_count = 3;
    for (int i = 0; i < test_count; i++) {
        LOG_INFO << "NNNNNNNNNNNNNNNN TestTCPClientReconnect i=" << i;
        tsrv.reset(new evpp::TCPServer(tcp_server_thread->loop(), addr, "tcp_server", i));
        tsrv->SetMessageCallback([](const evpp::TCPConnPtr& conn,
                                    evpp::Buffer* msg) {
            message_recved_count++;
        });
        auto rc = tsrv->Init();
        rc = rc && tsrv->Start();
        H_TEST_ASSERT(rc);
        usleep(evpp::Duration(2.0).Microseconds());
        tsrv->Stop();
        while (!tsrv->IsStopped()) {
            usleep(1);
        }
        tsrv.reset();
    }
    LOG_INFO << "XXXXXXXXXX connected_count=" << connected_count << " message_recved_count=" << message_recved_count;
    tcp_client_thread->loop()->RunInLoop([client]() {client->Disconnect(); });
    tcp_client_thread->loop()->RunAfter(evpp::Duration(1.0), [client]() {delete client; });
    usleep(evpp::Duration(2.0).Microseconds());
    client = nullptr;
    tcp_client_thread->Stop(true);
    tcp_server_thread->Stop(true);
    H_TEST_ASSERT(tcp_client_thread->IsStopped());
    H_TEST_ASSERT(tcp_server_thread->IsStopped());
    H_TEST_ASSERT(connected_count == test_count);
    H_TEST_ASSERT(message_recved_count == test_count);
    tcp_client_thread.reset();
    tcp_server_thread.reset();
    tsrv.reset();

    H_TEST_ASSERT(evpp::GetActiveEventCount() == 0);
}



