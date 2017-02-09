#pragma once

namespace {
struct OnApp {
    OnApp() {
#ifdef WIN32
        // Initialize net work.
        WSADATA wsaData;
        // Initialize Winsock 2.2
        int err = WSAStartup(MAKEWORD(2, 2), &wsaData);

        if (err) {
            std::cout << "WSAStartup() failed with error: %d" << err;
        }

        //std::cout << "WSAEWOULDBLOCK=" << WSAEWOULDBLOCK << std::endl;
#endif
        //std::cout << "EWOULDBLOCK=" << EWOULDBLOCK << " EAGAIN=" << EAGAIN << std::endl;
    }
    ~OnApp() {
#ifdef WIN32
        system("pause");
        WSACleanup();
#endif
    }
} __s_onexit_pause;
}
