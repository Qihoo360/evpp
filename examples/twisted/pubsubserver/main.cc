#include <set>

#include <evpp/exp.h>
#include <evpp/tcp_server.h>
#include <evpp/buffer.h>
#include <evpp/tcp_conn.h>

#include "../../echo/tcpecho/winmain-inl.h"

class Server {
public:
    Server(int port) {
        std::string addr = std::string("0.0.0.0:") + std::to_string(port);
        loop_.reset(new evpp::EventLoop);
        server_.reset(new evpp::TCPServer(loop_.get(), addr, "ChatRoom", 0));
        server_->SetMessageCallback(std::bind(&Server::OnMessage, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
        server_->SetConnectionCallback(std::bind(&Server::OnConnection, this, std::placeholders::_1));
    }

    void Run() {
        bool rc = server_->Init();
        rc = rc && server_->Start();
        assert(rc);
        loop_->Run();
    }

private:
    void OnMessage(const evpp::TCPConnPtr& conn,
                   evpp::Buffer* msg) {
        std::string s = msg->NextAllString();
        LOG_INFO << "Received a message [" << s << "]";
        if (s == "quit" || s == "exit") {
            conn->Close();
        }

        for (auto &c : conns_) {
            c->Send(s);
        }
    }

    void OnConnection(const evpp::TCPConnPtr& conn) {
        if (conn->IsConnected()) {
            LOG_INFO << "A new connection from " << conn->remote_addr() << " to " << server_->listen_addr() << " is UP";
            conns_.insert(conn);
        } else {
            LOG_INFO << "Disconnected from " << conn->remote_addr();
            conns_.erase(conn);
        }
    }

private:
    std::shared_ptr<evpp::EventLoop> loop_;
    std::shared_ptr<evpp::TCPServer> server_;
    std::set<evpp::TCPConnPtr> conns_;
};


int main(int argc, char* argv[]) {
    int port = 1025;
    if (argc == 2) {
        port = std::atoi(argv[1]);
    }
    
    Server s(port);
    s.Run();
    return 0;
}
