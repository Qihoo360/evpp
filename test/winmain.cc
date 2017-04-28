#include "test_common.h"

#include "evpp/libevent_headers.h"

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

#ifdef _WIN32
#pragma comment(lib, "evpp_static.lib")
#pragma comment(lib, "glog.lib")
#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "event.lib")
#pragma comment(lib, "event_core.lib") // libevent2.0
#pragma comment(lib, "event_extra.lib") // libevent2.0
#endif

// main function is defined on gtest_main.cc