#include "codec.h"

#include <evpp/event_loop.h>
#include <evpp/event_loop_thread.h>
#include <evpp/tcp_client.h>

#include <mutex>
#include <iostream>
#include <stdio.h>

class ChatClient {
public:
    ChatClient(evpp::EventLoop* loop, const std::string& serverAddr)
        : client_(loop, serverAddr, "ChatClient"),
        codec_(std::bind(&ChatClient::OnStringMessage, this, std::placeholders::_1, std::placeholders::_2)) {
        client_.SetConnectionCallback(
            std::bind(&ChatClient::OnConnection, this, std::placeholders::_1));
        client_.SetMessageCallback(
            std::bind(&LengthHeaderCodec::OnMessage, &codec_, std::placeholders::_1, std::placeholders::_2));
    }

    void Connect() {
        client_.Connect();
    }

    void Disconnect() {
        client_.Disconnect();
    }

    void Write(const evpp::Slice& message) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (connection_) {
            codec_.Send(connection_, message);
        }
    }

private:
    void OnConnection(const evpp::TCPConnPtr& conn) {
        LOG_INFO << conn->AddrToString() << " is " << (conn->IsConnected() ? "UP" : "DOWN");

        std::lock_guard<std::mutex> lock(mutex_);
        if (conn->IsConnected()) {
            connection_ = conn;
        } else {
            connection_.reset();
        }
    }

    void OnStringMessage(const evpp::TCPConnPtr&,
                         const std::string& message) {
        fprintf(stdout, "<<< %s\n", message.c_str());
        fflush(stdout);
    }

    evpp::TCPClient client_;
    LengthHeaderCodec codec_;
    std::mutex mutex_;
    evpp::TCPConnPtr connection_;
};

int main(int argc, char* argv[]) {
    if (argc != 3) {
        printf("Usage: %s host_ip port\n", argv[0]);
        return -1;
    }
    evpp::EventLoopThread loop;
    loop.Start(true);
    std::string host = argv[1];
    std::string port = argv[2];

    ChatClient client(loop.loop(), host + ":" + port);
    client.Connect();
    std::string line;
    while (std::getline(std::cin, line)) {
        if (line == "quit") {
            client.Disconnect();
            break;
        }
        client.Write(line);
    }
    loop.Stop(true);
    return 0;
}

