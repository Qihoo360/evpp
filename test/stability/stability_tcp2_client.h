#include <evpp/libevent.h>
#include <evpp/event_watcher.h>
#include <evpp/event_loop.h>
#include <evpp/event_loop_thread.h>
#include <evpp/tcp_server.h>
#include <evpp/buffer.h>
#include <evpp/tcp_conn.h>
#include <evpp/tcp_client.h>

void TestTCPClientConnectFailed() {
    std::shared_ptr<evpp::EventLoop> loop(new evpp::EventLoop);
    std::shared_ptr<evpp::TCPClient> client(new evpp::TCPClient(loop.get(), GetListenAddr(), "TCPPingPongClient"));
    client->SetConnectionCallback([&loop, &client](const evpp::TCPConnPtr& conn) {
        assert(!conn->IsConnected());
        client->Disconnect();
        loop->Stop();
    });
    client->set_auto_reconnect(false);
    client->Connect();
    loop->Run();
    client.reset();
    loop.reset();
    assert(evpp::GetActiveEventCount() == 0);
}

void TestTCPClientDisconnectImmediately() {
    std::shared_ptr<evpp::EventLoop> loop(new evpp::EventLoop);
    std::shared_ptr<evpp::TCPClient> client(new evpp::TCPClient(loop.get(), "cmake.org:80", "TCPPingPongClient"));
    client->SetConnectionCallback([loop, client](const evpp::TCPConnPtr& conn) {
        assert(!conn->IsConnected());
        auto f = [loop]() { loop->Stop(); };
        loop->RunAfter(1.0, f);
    });
    client->set_auto_reconnect(false);
    client->Connect();
    client->Disconnect();
    loop->Run();
    client.reset();
    loop.reset();
    assert(evpp::GetActiveEventCount() == 0);
}



namespace {
    struct NSQConn {
        NSQConn(evpp::EventLoop* loop) : loop_(loop) {
            client_ = std::make_shared<evpp::TCPClient>(loop, "www.so.com:80", "TCPPingPongClient");
            client_->SetConnectionCallback([this](const evpp::TCPConnPtr& conn) {
                assert(conn->IsConnected());
                assert(this->loop_->IsRunning());
                this->connected_ = true;
                client_->SetConnectionCallback(evpp::ConnectionCallback());
            });
            client_->Connect();
        }

        void Disconnect() {
            if (!connected_) {
                loop_->RunAfter(100.0, [this]() {this->Disconnect(); });
                return;
            }

            // We call TCPClient::Disconnect and then delete the hold object of TCPClient immediately
            client_->Disconnect();
            client_.reset();
            connected_ = false;
            loop_->RunAfter(100.0, [this]() {this->loop_->Stop(); });
        }

        std::shared_ptr<evpp::TCPClient> client_;
        bool connected_ = false;
        evpp::EventLoop* loop_;
    };
}

void TestTCPClientDisconnectAndDestruct() {
    std::shared_ptr<evpp::EventLoop> loop(new evpp::EventLoop);
    NSQConn nc(loop.get());
    loop->RunAfter(100.0, [&nc]() {nc.Disconnect(); });
    loop->Run();
    loop.reset();
    assert(evpp::GetActiveEventCount() == 0);
}

void TestTCPClientConnectLocalhost() {
    evpp::EventLoop loop;
    evpp::TCPClient client(&loop, GetListenAddr(), "TestClient");
    client.SetConnectionCallback([&loop, &client](const evpp::TCPConnPtr& conn) {
        assert(!conn->IsConnected());
        client.Disconnect();
        loop.Stop();
    });
    client.SetMessageCallback([](const evpp::TCPConnPtr& conn, evpp::Buffer* buf) {});
    client.Connect();
    loop.Run();
    assert(evpp::GetActiveEventCount() == 0);
}



