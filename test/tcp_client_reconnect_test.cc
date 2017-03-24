#include "test_common.h"

#include <evpp/exp.h>
#include <evpp/libevent_headers.h>
#include <evpp/libevent_watcher.h>
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
static void OnMessage(const evpp::TCPConnPtr& conn,
                      evpp::Buffer* msg) {
    message_recved_count++;
}

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
    client->SetConnectionCallback(&OnClientConnection);
    client->Connect();
    return client;
}

void DeleteTCPClient(evpp::TCPClient* client) {
    delete client;
}
}


TEST_UNIT(testTCPClientReconnect) {
    std::unique_ptr<evpp::EventLoopThread> tcp_client_thread(new evpp::EventLoopThread);
    tcp_client_thread->SetName("TCPClientThread");
    tcp_client_thread->Start(true);
    std::unique_ptr<evpp::EventLoopThread> tcp_server_thread(new evpp::EventLoopThread);
    tcp_server_thread->SetName("TCPServerThread");
    tcp_server_thread->Start(true);
    evpp::TCPClient* client = StartTCPClient(tcp_client_thread->event_loop());
    client->set_reconnect_interval(evpp::Duration(0.1));

    int test_count = 2;
    for (int i = 0; i < test_count; i++) {
        tsrv.reset(new evpp::TCPServer(tcp_server_thread->event_loop(), addr, "tcp_server", 1)); //TODO FIXME 修改为0个线程，会出现map/vector iterator崩溃
        tsrv->SetMessageCallback(&OnMessage);
        tsrv->Init()&& tsrv->Start();
        usleep(evpp::Duration(2.0).Microseconds());
        tsrv->Stop();
        usleep(evpp::Duration(2.0).Microseconds());
        tsrv.reset();
    }
    LOG_INFO << "XXXXXXXXXX connected_count=" << connected_count << " message_recved_count=" << message_recved_count;
    tcp_client_thread->event_loop()->RunInLoop(std::bind(&evpp::TCPClient::Disconnect, client));
    tcp_client_thread->event_loop()->RunAfter(evpp::Duration(1.0), std::bind(&DeleteTCPClient, client));
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



