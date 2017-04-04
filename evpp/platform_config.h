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

#if defined(__APPLE__)
#define H_OS_MACOSX
#endif

#ifdef _DEBUG
#ifndef H_DEBUG_MODE
#define H_DEBUG_MODE
#endif
#endif

#include <assert.h>
#include <stdint.h>

#ifdef __cplusplus
#include <iostream>
#include <memory>
#include <functional>
#endif // end of define __cplusplus

#ifdef H_OS_WINDOWS
#define usleep(us) Sleep((us)/1000)
#define snprintf  _snprintf
#define thread_local __declspec( thread )
#endif

#ifdef H_OS_WINDOWS
#pragma warning( disable: 4005 ) // warning C4005 : 'va_copy' : macro redefinition
#pragma warning( disable: 4251 )
#pragma warning( disable: 4996 ) // warning C4996: 'strerror': This function or variable may be unsafe. Consider using strerror_s instead. To disable deprecation, use _CRT_SECURE_NO_WARNINGS. See online help for details.
#pragma warning( disable: 4244 4251 4355 4715 4800 4996 4005 4819)
#pragma warning( disable: 4530 ) // C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\include\xlocale(341): warning C4530: C++ exception handler used, but unwind semantics are not enabled. Specify /EHsc
#pragma warning( disable: 4577 ) // C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\include\exception(359): warning C4577: 'noexcept' used with no exception handling mode specified; termination on exception is not guaranteed. Specify /EHsc
#pragma warning( disable: 4503 ) // c:\program files (x86)\microsoft visual studio 14.0\vc\include\functional(357): warning C4503: '__LINE__Var': decorated name length exceeded, name was truncated
#endif

// get rid of Windows/Linux inconsistencies
#ifdef H_OS_WINDOWS
#   ifndef PRIu64
#       define PRIu64     "I64u"
#   endif
#else
#   ifndef PRIu64
#       define PRIu64     "lu"
#   endif
#endif
