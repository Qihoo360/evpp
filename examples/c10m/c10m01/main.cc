#include <evpp/tcp_server.h>
#include <evpp/buffer.h>
#include <evpp/tcp_conn.h>

#ifdef _WIN32
#include "../../winmain-inl.h"
#endif

typedef std::map<uint64_t, evpp::TCPConnPtr> ConnectionsMap;
typedef std::shared_ptr<ConnectionsMap> ConnectionsMapPtr;

std::atomic<int> g_connected = {0}; // The all connected connection count
std::atomic<int> g_current_round_connected = {0};
std::atomic<int> g_current_round_disconnected = {0};

std::atomic<int64_t> g_recved_message = {0};
std::atomic<int64_t> g_current_round_recved_message = {0};

std::atomic<int64_t> g_current_round_recved_bytes = {0};

void Print() {
    LOG_ERROR << "Running ...\n"
        << "\t                Total connected " << g_connected << "\n"
        << "\t        Current round connected " << g_current_round_connected.exchange(0) << "\n"
        << "\t     Current round disconnected " << g_current_round_disconnected.exchange(0) << "\n"
        << "\t        Total received messages " << g_recved_message << "\n"
        << "\tCurrent round received messages " << g_current_round_recved_message.exchange(0) << "\n"
        << "\t                     Throughput " << g_current_round_recved_bytes.exchange(0) / 1024.0 / 1024.0 << "MB/s\n";
}

void OnMessage(const evpp::TCPConnPtr& conn,
               evpp::Buffer* msg) {
    const size_t kHeadLen = 4;
    while (msg->size() >= kHeadLen) {
        int32_t len = msg->PeekInt32();
        if (msg->size() < len + kHeadLen) {
            break;
        }

        conn->Send(msg->data(), len + kHeadLen);
        g_current_round_recved_bytes += len + kHeadLen;
        g_recved_message++;
        g_current_round_recved_message++;
        msg->Skip(kHeadLen);
        std::string m = msg->NextString(len);
        bool check = true;
        for (auto i : m) {
            if (i != 'a') {
                check = false;
                break;
            }
        }
        if (!check) {
            LOG_ERROR << "Received an ERROR message.";
        }
    }
}

void OnConnection(const evpp::TCPConnPtr& conn) {
    if (conn->IsConnected()) {
        LOG_INFO << "Accept a new connection " << conn->AddrToString();
        g_connected++;
        g_current_round_connected++;
    } else {
        LOG_INFO << "Disconnected from " << conn->remote_addr();
        g_connected--;
        g_current_round_disconnected++;
    }
}

int main(int argc, char* argv[]) {
    std::string port = "9099";
    if (argc == 2) {
        port = argv[1];
    }
    std::string addr = std::string("0.0.0.0:") + port;
    evpp::EventLoop loop;
    loop.RunEvery(evpp::Duration(1.0), &Print);
    evpp::TCPServer server(&loop, addr, "c10m", 23);
    server.SetMessageCallback(&OnMessage);
    server.SetConnectionCallback(&OnConnection);
    server.Init();
    server.Start();
    loop.Run();
    return 0;
}
