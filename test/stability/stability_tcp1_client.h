
#include <evpp/libevent_headers.h>
#include <evpp/event_watcher.h>
#include <evpp/event_loop.h>
#include <evpp/event_loop_thread.h>
#include <evpp/tcp_server.h>
#include <evpp/buffer.h>
#include <evpp/tcp_conn.h>
#include <evpp/tcp_client.h>

namespace {
    static std::shared_ptr<evpp::TCPServer> tsrv;
    static std::atomic<int> connected_count(0);
    static std::atomic<int> message_recved_count(0);

    void OnClientConnection1(const evpp::TCPConnPtr& conn) {
        if (conn->IsConnected()) {
            conn->Send("hello");
            LOG_INFO << "Send a message to server when connected.";
            connected_count++;
        } else {
            LOG_INFO << "Disconnected from " << conn->remote_addr();
        }
    }

    evpp::TCPClient* StartTCPClient1(evpp::EventLoop* loop) {
        evpp::TCPClient* client(new evpp::TCPClient(loop, GetListenAddr(), "TCPPingPongClient"));
        client->set_reconnect_interval(evpp::Duration(0.1));
        client->SetConnectionCallback(&OnClientConnection1);
        client->Connect();
        return client;
    }

}

void TestTCPClientReconnect() {
    tsrv.reset();
    connected_count = (0);
    message_recved_count = (0);
    std::unique_ptr<evpp::EventLoopThread> tcp_client_thread(new evpp::EventLoopThread);
    tcp_client_thread->set_name("TCPClientThread");
    tcp_client_thread->Start(true);
    std::unique_ptr<evpp::EventLoopThread> tcp_server_thread(new evpp::EventLoopThread);
    tcp_server_thread->set_name("TCPServerThread");
    tcp_server_thread->Start(true);
    evpp::TCPClient* client = StartTCPClient1(tcp_client_thread->loop());

    int test_count = 3;
    for (int i = 0; i < test_count; i++) {
        LOG_INFO << "NNNNNNNNNNNNNNNN TestTCPClientReconnect i=" << i;
        tsrv.reset(new evpp::TCPServer(tcp_server_thread->loop(), GetListenAddr(), "tcp_server", i));
        tsrv->SetMessageCallback([](const evpp::TCPConnPtr& conn,
                                    evpp::Buffer* msg) {
            message_recved_count++;
        });
        bool rc = tsrv->Init();
        assert(rc);
        rc = tsrv->Start();
        assert(rc);
        (void)rc;
        usleep(evpp::Duration(2.0).Microseconds()); // sleep 2 seconds to let the TCP client connected.
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
    assert(tcp_client_thread->IsStopped());
    assert(tcp_server_thread->IsStopped());
    assert(connected_count == test_count);
    assert(message_recved_count == test_count);
    tcp_client_thread.reset();
    tcp_server_thread.reset();
    tsrv.reset();

    assert(evpp::GetActiveEventCount() == 0);
}



