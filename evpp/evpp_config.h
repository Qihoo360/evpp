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
#include "evpp_config.w32.h"
#endif

#ifndef H_OS_WINDOWS
#define EVPP_EXPORT

// get rid of Windows/Linux inconsistencies
#ifndef PRIu64
#    define PRIu64     "lu"
#endif

#endif
