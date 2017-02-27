#pragma once

//! Define Operation System.
#if ( defined(WIN32) || defined(WIN64) )
#   ifndef H_OS_WINDOWS
#       define H_OS_WINDOWS
#   endif
#   ifndef H_WINDOWS_API
#       define H_WINDOWS_API
#   endif
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
