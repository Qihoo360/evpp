
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

            std::cout << "EWOULDBLOCK=" << EWOULDBLOCK << " WSAEWOULDBLOCK=" << WSAEWOULDBLOCK << " EAGAIN=" << EAGAIN << std::endl;
        }
        ~OnApp() {
            system("pause");
        }
    } __s_onexit_pause;
#endif
}
