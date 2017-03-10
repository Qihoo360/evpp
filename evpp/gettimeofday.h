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
    time_t clock;
    struct tm tm;
    SYSTEMTIME wtm;
    GetLocalTime(&wtm);
    tm.tm_year = wtm.wYear - 1900;
    tm.tm_mon = wtm.wMonth - 1;
    tm.tm_mday = wtm.wDay;
    tm.tm_hour = wtm.wHour;
    tm.tm_min = wtm.wMinute;
    tm.tm_sec = wtm.wSecond;
    tm.tm_isdst = -1;
    clock = mktime(&tm);
    tp->tv_sec = (long)clock;
    tp->tv_usec = wtm.wMilliseconds * 1000;
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

