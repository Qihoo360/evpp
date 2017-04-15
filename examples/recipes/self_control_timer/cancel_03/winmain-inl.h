#pragma once

#ifdef _WIN32
#include <WinSock2.h>
#endif

#include <iostream>

namespace {
struct OnApp {
    OnApp() {
#ifdef WIN32
        // Initialize Winsock 2.2
        WSADATA wsaData;
        int err = WSAStartup(MAKEWORD(2, 2), &wsaData);

        if (err) {
            std::cout << "WSAStartup() failed with error: %d" << err;
        }
#endif
    }
    ~OnApp() {
#ifdef WIN32
        system("pause");
        WSACleanup();
#endif
    }
} __s_onexit_pause;
}


#ifdef _WIN32
#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "event.lib")
#pragma comment(lib, "event_core.lib") // libevent2.0
#pragma comment(lib, "event_extra.lib") // libevent2.0
#endif
