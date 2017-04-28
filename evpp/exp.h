#pragma once

// We must link against these libraries on windows platform for Visual Studio IDE

#ifdef _WIN32
#pragma comment(lib, "evpp_static.lib")
#pragma comment(lib, "glog.lib")
#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "event.lib")
#pragma comment(lib, "event_core.lib") // libevent2.0
#pragma comment(lib, "event_extra.lib") // libevent2.0
#endif




