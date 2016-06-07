#pragma once

//! Define Operation System.
#if ( defined(WIN32) || defined(WIN64) )
#	ifndef H_OS_WINDOWS
#		define H_OS_WINDOWS
#	endif
#	ifndef H_WINDOWS_API
#		define H_WINDOWS_API
#	endif
#endif

//! Module symbol export
#ifdef H_WINDOWS_API
#	ifndef  H_STATIC_LIB_LIBEVENTPP
#		ifdef  H_LIBEVENTPP_BASE_EXPORTS
#			define EVPPBASE_EXPORT __declspec(dllexport)
#		else
#			define EVPPBASE_EXPORT __declspec(dllimport)
#		endif
#	else
#		define EVPPBASE_EXPORT
#	endif
#else
#	define EVPPBASE_EXPORT
#endif // H_STATIC_LIB_

#define NET_EXPORT EVPPBASE_EXPORT

