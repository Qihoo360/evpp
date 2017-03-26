#pragma once

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
