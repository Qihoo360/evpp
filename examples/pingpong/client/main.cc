#include <evpp/exp.h>
#include <evpp/tcp_client.h>
#include <evpp/buffer.h>
#include <evpp/tcp_conn.h>

std::string message(512, 'x');

std::atomic<uint64_t> recv_size(0);
std::atomic<uint64_t> send_size(0);
std::atomic<uint64_t> recv_count(0);
std::atomic<uint64_t> send_count(0);


void OnMessage(const evpp::TCPConnPtr& conn,
               evpp::Buffer* msg,
               evpp::Timestamp ts) {
    //LOG_TRACE << "Recv a message size=" << msg->length();
    recv_count++;
    recv_size.fetch_add(msg->size());

    for (;;) {
        if (msg->length() < 4) {
            return;
        }

        int32_t len = msg->PeekInt32();

        if ((int32_t)(msg->length()) < len + 4) {
            return;
        }

        //LOG_INFO << "Received a message size=" << len;
        conn->Send(msg->data(), len + 4);
        send_size.fetch_add(len + 4);
        send_count++;
        msg->Retrieve(4 + len);
    }

    usleep(1000 * 1000);
}

void Print() {
    LOG_INFO << "recv_count=" << recv_count << " recv_size=" << recv_size << " send_count=" << send_count << " send_size=" << send_size;
}

void OnConnection(const evpp::TCPConnPtr& conn) {
    if (conn->IsConnected()) {
        evpp::Buffer buf;
        buf.Append(message);
        buf.PrependInt32(int32_t(message.size()));
        send_size.fetch_add(buf.size());
        send_count++;
        conn->Send(&buf);
        conn->loop()->RunEvery(evpp::Duration(1.0), &Print);
    } else {
        LOG_INFO << "Disconnected from " << conn->remote_addr();
    }
}


int main(int argc, char* argv[]) {
    std::string addr = "127.0.0.1:9099";

    if (argc == 2) {
        addr = argv[1];
    }

    evpp::EventLoop loop;
    evpp::TCPClient client(&loop, addr, "TCPPingPongClient");
    client.SetMessageCallback(&OnMessage);
    client.SetConnectionCallback(&OnConnection);
    client.Connect();
    loop.Run();
    return 0;
}

#ifdef WIN32
#include "../../echo/tcpecho/winmain-inl.h"
#endif
