/********************************************************************
 *	Created:	2014/03/06 14:19
 *	File name: 	libevent_headers.h
 *	Author:		weizili
 *	Purpose:	
 *
 *	Copyright 2010-2013, All Rights Reserved.
 ********************************************************************/
#pragma once

#ifdef WIN32
	#include <WinSock2.h>
	#include <event/event.h>
	#include <event/evhttp.h>
    #include <event/evutil.h>
    #define _LIBEVENT14 1
#else
	#include <sys/queue.h>
    #ifdef _LIBEVENT14
        #include <event.h>
        #include <evhttp.h>
        #include <event/evutil.h>
    #else
        #include <event2/event.h>
        #include <event2/event_struct.h>
        #include <event2/buffer.h>
        #include <event2/bufferevent.h>
        #include <event2/http.h>
        #include <event2/http_struct.h>
        #include <event2/event_compat.h>
	#endif
#endif

#ifndef evtimer_new
#define evtimer_new(b, cb, arg)	       event_new((b), -1, 0, (cb), (arg))
#endif

#ifdef _LIBEVENT14
extern "C" {
    struct event * event_new(struct event_base *base, int fd, short events, void(*cb)(int, short, void *), void *arg);
    void event_free(struct event *ev);
}
#endif
