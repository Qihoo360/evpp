#include <evpp/exp.h>
#include <evpp/tcp_server.h>
#include <evpp/buffer.h>
#include <evpp/tcp_conn.h>

void OnMessage(const evpp::TcpConnectionPtr& conn,
               evpp::Buffer* msg,
               evpp::base::Timestamp ts) {
    std::string s = msg->NextAllString();
    LOG_INFO << "Received a message [" << s << "]";
    conn->Send(s.data(), s.size());
}

int main(int argc, char* argv[]) {
    LOG_INFO << "EWOULDBLOCK=" << EWOULDBLOCK << " WSAEWOULDBLOCK=" << WSAEWOULDBLOCK << " EAGAIN=" << EAGAIN;

    std::string port = "9099";
    if (argc == 2) {
        port = argv[1];
    }
    std::string addr = std::string("127.0.0.1:") + port;
    evpp::EventLoop loop;
    evpp::TCPServer server(&loop, addr, "TCPEcho", 1);
    server.SetMesageHandler(&OnMessage);
    server.Start();
    loop.Run();
    return 0;
}





#ifdef WIN32
#   ifdef _DEBUG
#		pragma comment(lib,"libevent_d.lib")
#   else
#		pragma comment(lib,"libevent.lib")
#   endif
#	pragma comment(lib,"Ws2_32.lib")
#	pragma comment(lib,"libglog_static.lib")
#endif

namespace {
#ifdef WIN32
    struct OnApp {
        OnApp() {
            // Initialize net work.
            WSADATA wsaData;
            // Initialize Winsock 2.2
            int nError = WSAStartup(MAKEWORD(2, 2), &wsaData);

            if (nError) {
                std::cout << "WSAStartup() failed with error: %d" << nError;
            }

        }
        ~OnApp() {
            system("pause");
        }
    } __s_onexit_pause;
#endif
}