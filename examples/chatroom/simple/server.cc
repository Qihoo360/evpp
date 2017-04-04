#include "codec.h"

#include <evpp/event_loop.h>
#include <evpp/tcp_server.h>

#include <mutex>
#include <iostream>
#include <stdio.h>
#include <set>

class ChatServer {
public:
    ChatServer(evpp::EventLoop* loop,
               const std::string& addr)
        : server_(loop, addr, "ChatServer", 0),
        codec_(std::bind(&ChatServer::OnStringMessage, this, std::placeholders::_1, std::placeholders::_2)) {
        server_.SetConnectionCallback(
            std::bind(&ChatServer::OnConnection, this, std::placeholders::_1));
        server_.SetMessageCallback(
            std::bind(&LengthHeaderCodec::OnMessage, &codec_, std::placeholders::_1, std::placeholders::_2));
    }

    void Start() {
        server_.Init();
        server_.Start();
    }

private:
    void OnConnection(const evpp::TCPConnPtr& conn) {
        LOG_INFO << conn->AddrToString() << " is " << (conn->IsConnected() ? "UP" : "DOWN");
        if (conn->IsConnected()) {
            connections_.insert(conn);
        } else {
            connections_.erase(conn);
        }
    }

    void OnStringMessage(const evpp::TCPConnPtr&,
                         const std::string& message) {
        for (ConnectionList::iterator it = connections_.begin();
             it != connections_.end();
             ++it) {
            codec_.Send(*it, message);
        }
    }

    typedef std::set<evpp::TCPConnPtr> ConnectionList;
    evpp::TCPServer server_;
    LengthHeaderCodec codec_;
    ConnectionList connections_;
};

int main(int argc, char* argv[]) {
    if (argc != 2) {
        printf("Usage: %s port\n", argv[0]);
        return -1;
    }

    evpp::EventLoop loop;
    std::string addr = std::string("0.0.0.0:") + argv[1];
    ChatServer server(&loop, addr);
    server.Start();
    loop.Run();
    return 0;
}

