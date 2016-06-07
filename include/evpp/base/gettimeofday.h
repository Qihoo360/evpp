#pragma once

#include <time.h>

#ifdef WIN32
#define _WINSOCKAPI_  
#include <windows.h>
#include <WinSock2.h>
#else
#include <sys/time.h>
#endif

#ifdef WIN32

//thrift-0.8.0 implementation
#if 0

#if defined(_MSC_VER) || defined(_MSC_EXTENSIONS)
#   define DELTA_EPOCH_IN_MICROSECS  11644473600000000Ui64
#else                                                                                                                 
#   define DELTA_EPOCH_IN_MICROSECS  11644473600000000ULL                                                             
#endif                                                                                                                

struct timezone
{
    int  tz_minuteswest; /* minutes W of Greenwich */
    int  tz_dsttime;     /* type of dst correction */
};

inline int gettimeofday(struct timeval * tv, struct timezone * tz)
{
    FILETIME         ft;
    unsigned __int64 tmpres(0);
    static int       tzflag;

    if (NULL != tv)
    {
        GetSystemTimeAsFileTime(&ft);

        tmpres |= ft.dwHighDateTime;
        tmpres <<= 32;
        tmpres |= ft.dwLowDateTime;

        /*converting file time to unix epoch*/
        tmpres -= DELTA_EPOCH_IN_MICROSECS;
        tmpres /= 10;  /*convert into microseconds*/
        tv->tv_sec = (long)(tmpres / 1000000UL);
        tv->tv_usec = (long)(tmpres % 1000000UL);
    }

    if (NULL != tz)
    {
        if (!tzflag)
        {
            _tzset();
            tzflag++;
        }

        long time_zone(0);
        errno_t err(_get_timezone(&time_zone));
        if (err == NO_ERROR)
        {
            tz->tz_minuteswest = time_zone / 60;
        }
        else
        {
            return -1;
        }

        int day_light(0);
        err = (_get_daylight(&day_light));
        if (err == NO_ERROR)
        {
            tz->tz_dsttime = day_light;
            return 0;
        }
        else
        {
            return -1;
        }
    }

    return -1;
}
#else
inline int gettimeofday(struct timeval *tp, void *tzp)
{
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
#endif

#endif //end of WIN32

namespace evpp
{
    inline double utcsecond()
    {
        struct timeval tv;
        gettimeofday(&tv, NULL);
        return (double)(tv.tv_sec) + ((double)(tv.tv_usec)) / 1000000.0f;
    }

    inline uint64_t utcmicrosecond()
    {
        struct timeval tv;
        gettimeofday(&tv, NULL);
        return (uint64_t)(((uint64_t)(tv.tv_sec)) * 1000000 + tv.tv_usec);
    }

    inline struct timeval timevalconv(uint64_t time_us) {
        struct timeval tv;
        tv.tv_sec = (long)time_us / 1000000;
        tv.tv_usec = (long)time_us % 1000000;
        return tv;
    }
}

