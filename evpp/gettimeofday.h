#pragma once

#include <time.h>
#include <stdint.h>

#ifdef WIN32
#define _WINSOCKAPI_
#include <windows.h>
#include <WinSock2.h>
#else
#include <sys/time.h>
#endif

#ifdef WIN32

#ifndef H_GETTIMEOFDAY
#define H_GETTIMEOFDAY
inline int gettimeofday(struct timeval* tp, void* tzp) {
	uint64_t  intervals;
	FILETIME  ft;

	GetSystemTimeAsFileTime(&ft);

	/*
	* A file time is a 64-bit value that represents the number
	* of 100-nanosecond intervals that have elapsed since
	* January 1, 1601 12:00 A.M. UTC.
	*
	* Between January 1, 1970 (Epoch) and January 1, 1601 there were
	* 134744 days,
	* 11644473600 seconds or
	* 11644473600,000,000,0 100-nanosecond intervals.
	*
	* See also MSKB Q167296.
	*/

	intervals = ((uint64_t)ft.dwHighDateTime << 32) | ft.dwLowDateTime;
	intervals -= 116444736000000000;

	tp->tv_sec = (long)(intervals / 10000000);
	tp->tv_usec = (long)((intervals % 10000000) / 10);


    return (0);
}
#endif // end of H_GETTIMEOFDAY

#endif //end of WIN32

namespace evpp {
inline double utcsecond() {
    struct timeval tv;
    gettimeofday(&tv, nullptr);
    return (double)(tv.tv_sec) + ((double)(tv.tv_usec)) / 1000000.0f;
}

inline uint64_t utcmicrosecond() {
    struct timeval tv;
    gettimeofday(&tv, nullptr);
    return (uint64_t)(((uint64_t)(tv.tv_sec)) * 1000000 + tv.tv_usec);
}

inline struct timeval timevalconv(uint64_t time_us) {
    struct timeval tv;
    tv.tv_sec = (long)time_us / 1000000;
    tv.tv_usec = (long)time_us % 1000000;
    return tv;
}
}

