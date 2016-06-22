#pragma once

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
    struct OnApp {
        OnApp() {
#ifdef WIN32
            // Initialize net work.
            WSADATA wsaData;
            // Initialize Winsock 2.2
            int nError = WSAStartup(MAKEWORD(2, 2), &wsaData);

            if (nError) {
                std::cout << "WSAStartup() failed with error: %d" << nError;
            }
            std::cout << "WSAEWOULDBLOCK=" << WSAEWOULDBLOCK << std::endl;
#endif
            std::cout << "EWOULDBLOCK=" << EWOULDBLOCK << " EAGAIN=" << EAGAIN << std::endl;
        }
        ~OnApp() {
#ifdef WIN32
            system("pause");
            WSACleanup();
#endif
        }
    } __s_onexit_pause;
}
evutil_snprintf()
event_log