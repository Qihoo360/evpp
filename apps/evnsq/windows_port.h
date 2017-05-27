#pragma once

#include <evpp/windows_port.h>

// We must link against these libraries on windows platform for Visual Studio IDE
#ifdef _WIN32
#ifndef EVNSQ_EXPORTS
#pragma comment(lib, "evnsq_static.lib")
#endif
#endif


//! Module symbol export
// #ifdef H_WINDOWS_API
// #   ifndef  H_STATIC_LIB_LIBEVNSQ
// #       ifdef  EVNSQ_EXPORTS
// #           define EVNSQ_EXPORT __declspec(dllexport)
// #       else
// #           define EVNSQ_EXPORT __declspec(dllimport)
// #       endif
// #   else
// #       define EVNSQ_EXPORT
// #   endif
// #else
// #   define EVNSQ_EXPORT
// #endif // H_STATIC_LIB_

#define EVNSQ_EXPORT