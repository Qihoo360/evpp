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

//! Finds the compiler type and version.
#if defined( _MSC_VER )
#   ifndef H_COMPILER_MSVC
#       define H_COMPILER_MSVC
#   endif
#   ifndef H_COMPILER_VERSION
#       define H_COMPILER_VERSION _MSC_VER
#   endif
#   if H_COMPILER_VERSION < 1300
#       pragma error "Not supported compiler version. Abort! Abort!"
#   endif
#elif defined( __GNUC__ )
#   ifndef H_COMPILER_GNUC
#       define H_COMPILER_GNUC
#   endif
#   ifndef H_COMPILER_VERSION
#       define H_COMPILER_VERSION (((__GNUC__)*100) + \
                                   (__GNUC_MINOR__ * 10) + \
                                   __GNUC_PATCHLEVEL__)
#   endif
#else
#   pragma error "No known compiler. Abort! Abort!"
#endif

#endif // end of define __cplusplus

//////////////////////////////////////////////////////////////////////////
//                              Link Helper Macro                       //
//  Use: H_LINK_LIB(libname) to import a library.                       //
//////////////////////////////////////////////////////////////////////////
//------------------------------------------------------
#ifdef H_OS_WINDOWS
#   define H_LINK_OS_FLAG 1
#else
#   define H_LINK_OS_FLAG 0
#endif

#ifdef H_DEBUG_MODE
#   define H_LINK_FILE_DEBUG_FLAG 1
#else
#   define H_LINK_FILE_DEBUG_FLAG 0
#endif

//#define H_LINK_FILE_DEBUG_1( filename )   __pragma ( comment( lib , filename##"_d"##".lib" ))
#ifndef H_LINK_FILE_DEBUG_1
#   define H_LINK_FILE_DEBUG_1( filename ) __pragma (comment( lib , filename##".lib" ))
#endif
#ifndef H_LINK_FILE_DEBUG_0
#   define H_LINK_FILE_DEBUG_0( filename ) __pragma (comment( lib , filename##".lib" ))
#endif

#ifndef H_LINK_FILE_DEBUG_P
#   define H_LINK_FILE_DEBUG_P(filename,y)   H_LINK_FILE_DEBUG_##y(filename)
#endif

#ifndef H_LINK_FILE_0
#   define H_LINK_FILE_0(filename,y)
#endif
#ifndef H_LINK_FILE_1
#   define H_LINK_FILE_1(filename,y)         H_LINK_FILE_DEBUG_P(filename,y)
#endif

#ifndef H_LINK_FILE_PP
#   define H_LINK_FILE_PP( filename , sys )  H_LINK_FILE_##sys( filename , H_LINK_FILE_DEBUG_FLAG )
#endif
#ifndef H_LINK_FILE_P
#   define H_LINK_FILE_P( filename , sys )   H_LINK_FILE_PP( filename , sys )
#endif

#ifndef H_LINK_LIB
#   define H_LINK_LIB( filename )           H_LINK_FILE_P( filename , H_LINK_OS_FLAG )
#endif

// from google3/base/basictypes.h
// The H_ARRAYSIZE(arr) macro returns the # of elements in an array arr.
// The expression is a compile-time constant, and therefore can be
// used in defining new arrays, for example.
//
// H_ARRAYSIZE catches a few type errors.  If you see a compiler error
//
//   "warning: division by zero in ..."
//
// when using H_ARRAYSIZE, you are (wrongfully) giving it a pointer.
// You should only use H_ARRAYSIZE on statically allocated arrays.
//
// The following comments are on the implementation details, and can
// be ignored by the users.
//
// ARRAYSIZE(arr) works by inspecting sizeof(arr) (the # of bytes in
// the array) and sizeof(*(arr)) (the # of bytes in one array
// element).  If the former is divisible by the latter, perhaps arr is
// indeed an array, in which case the division result is the # of
// elements in the array.  Otherwise, arr cannot possibly be an array,
// and we generate a compiler error to prevent the code from
// compiling.
//
// Since the size of bool is implementation-defined, we need to cast
// !(sizeof(a) & sizeof(*(a))) to size_t in order to ensure the final
// result has type size_t.
//
// This macro is not perfect as it wrongfully accepts certain
// pointers, namely where the pointer size is divisible by the pointer
// size.  Since all our code has to go through a 32-bit compiler,
// where a pointer is 4 bytes, this means all pointers to a type whose
// size is 3 or greater than 4 will be (righteously) rejected.
//
// Kudos to Jorg Brown for this simple and elegant implementation.
#undef H_ARRAYSIZE
#define H_ARRAYSIZE(a) \
    ((sizeof(a) / sizeof(*(a))) / \
     static_cast<size_t>(!(sizeof(a) % sizeof(*(a)))))


#ifdef H_OS_WINDOWS
#include "evpp_config.w32.h"
#endif

#ifndef H_OS_WINDOWS
#define EVPP_EXPORT

// get rid of Windows/Linux inconsistencies
#ifndef PRIu64
#   define PRIu64     "lu"
#endif

#endif
