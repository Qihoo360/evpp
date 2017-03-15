#include <evpp/exp.h>
#include <evpp/tcp_server.h>
#include <evpp/buffer.h>
#include <evpp/tcp_conn.h>

#include "header.h"

uint32_t g_total_count = 100;

bool check_count(Header* header) {
    return (ntohl(header->packet_count_) >= g_total_count);
}

void OnConnection(const evpp::TCPConnPtr& conn) {
    if (conn->IsConnected()) {
        conn->SetTCPNoDelay(true);
        std::shared_ptr<uint32_t> count(new uint32_t(0));
        conn->set_context(evpp::Any(count));
    }
}

void OnMessage(const evpp::TCPConnPtr& conn,
               evpp::Buffer* buf) {
    LOG_INFO << " buf->size=" << buf->size();
    const size_t kHeaderLen = sizeof(Header);
    while (buf->size() > kHeaderLen) {
        Header* header = reinterpret_cast<Header*>(const_cast<char*>(buf->data()));
        auto full_size = header->get_full_size();
        if (buf->size() < full_size) {
            // need to read more data
            return;
        }

        LOG_INFO << "full_size=" << full_size << " header.body_size_=" << ntohl(header->body_size_) << " header.packet_count_=" << ntohl(header->packet_count_);

        if (check_count(header)) {
            conn->Close();
        } else {
            header->inc_packet_count();
            conn->Send(buf->data(), full_size);
            buf->Skip(full_size);
        }
    }

}

int main(int argc, char* argv[]) {
    std::string addr = "0.0.0.0:9099";
    int thread_num = 4;

    if (argc != 1 && argc != 4) {
        printf("Usage: %s <port> <thread-num> <total-count>\n", argv[0]);
        return 0;
    }

    if (argc == 4) {
        addr = std::string("0.0.0.0:") + argv[1];
        thread_num = atoi(argv[2]);
        g_total_count = atoi(argv[3]);
    }

    evpp::EventLoop loop;
    evpp::TCPServer server(&loop, addr, "TCPPingPongServer", thread_num);
    server.SetMessageCallback(&OnMessage);
    server.SetConnectionCallback(&OnConnection);
    server.Init();
    server.Start();
    loop.Run();
    return 0;
}


#include "../../../examples/echo/tcpecho/winmain-inl.h"


