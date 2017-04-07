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
    static bool connected = false;
    static bool message_recved = false;
    const static std::string tcp_listen_addr = "127.0.0.1:19099";

    void OnClientConnection(const evpp::TCPConnPtr& conn) {
        if (conn->IsConnected()) {
            conn->Send("hello");
            LOG_INFO << "Send a message to server when connected.";
            connected = true;
        } else {
            LOG_INFO << "Disconnected from " << conn->remote_addr();
        }
    }

    std::shared_ptr<evpp::TCPClient> StartTCPClient(evpp::EventLoop* loop) {
        std::shared_ptr<evpp::TCPClient> client(new evpp::TCPClient(loop, tcp_listen_addr, "TCPPingPongClient"));
        client->set_reconnect_interval(evpp::Duration(0.01));
        client->SetConnectionCallback(&OnClientConnection);
        client->Connect();
        loop->RunAfter(evpp::Duration(1.0), std::bind(&evpp::TCPClient::Disconnect, client));
        return client;
    }
}


void TestTCPServer1() {
    std::unique_ptr<evpp::EventLoopThread> tcp_client_thread(new evpp::EventLoopThread);
    tcp_client_thread->SetName("TCPClientThread");
    tcp_client_thread->Start();
    std::unique_ptr<evpp::EventLoop> loop(new evpp::EventLoop);
    std::unique_ptr<evpp::TCPServer> tsrv(new evpp::TCPServer(loop.get(), tcp_listen_addr, "tcp_server", 2));
    tsrv->SetMessageCallback([](const evpp::TCPConnPtr& conn,
                                evpp::Buffer* msg) {
        message_recved = true;
    });
    bool rc = tsrv->Init();
    assert(rc);
    rc = tsrv->Start();
    assert(rc);
    loop->RunAfter(evpp::Duration(1.4), [&tsrv]() { tsrv->Stop(); });
    loop->RunAfter(evpp::Duration(1.6), [&loop]() { loop->Stop(); });
    std::shared_ptr<evpp::TCPClient> client = StartTCPClient(tcp_client_thread->event_loop());
    loop->Run();
    tcp_client_thread->Stop(true);
    assert(!loop->running());
    assert(tcp_client_thread->IsStopped());
    assert(connected);
    assert(message_recved);
    tcp_client_thread.reset();
    loop.reset();
    tsrv.reset();
    assert(evpp::GetActiveEventCount() == 0);
}

void TestTCPServerSilenceShutdown1() {
    std::unique_ptr<evpp::EventLoop> loop(new evpp::EventLoop);
    std::unique_ptr<evpp::TCPServer> tsrv(new evpp::TCPServer(loop.get(), tcp_listen_addr, "tcp_server", 2));
    bool rc = tsrv->Init();
    assert(rc);
    rc = tsrv->Start();
    assert(rc);
    loop->RunAfter(evpp::Duration(1.2), [&tsrv]() { tsrv->Stop(); });
    loop->RunAfter(evpp::Duration(1.3), [&loop]() { loop->Stop(); });
    loop->Run();
    loop.reset();
    tsrv.reset();
    assert(evpp::GetActiveEventCount() == 0);
}

void TestTCPServerSilenceShutdown2() {
    std::unique_ptr<evpp::EventLoop> loop(new evpp::EventLoop);
    std::unique_ptr<evpp::TCPServer> tsrv(new evpp::TCPServer(loop.get(), tcp_listen_addr, "tcp_server", 2));
    bool rc = tsrv->Init();
    assert(rc);
    rc = tsrv->Start();
    assert(rc);
    loop->RunAfter(evpp::Duration(1.0), [&tsrv, &loop]() { tsrv->Stop(); loop->Stop(); });
    loop->Run();
    loop.reset();
    tsrv.reset();
    assert(evpp::GetActiveEventCount() == 0);
}

