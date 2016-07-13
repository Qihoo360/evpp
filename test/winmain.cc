#include "test_common.h"

#include "evpp/exp.h"
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

// main function is defined on gtest_main.cc