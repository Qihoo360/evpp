#pragma once

// We must link against these libraries on windows platform for Visual Studio IDE

#ifdef _WIN32
#pragma comment(lib, "evpp_static.lib")
#pragma comment(lib, "Ws2_32.lib")
#endif




