/********************************************************************
 *  Created:    2014/03/06 14:19
 *  File name:  libevent.h
 *  Author:     weizili
 *  Purpose:
 *
 *  Copyright 2010-2013, All Rights Reserved.
 ********************************************************************/
#pragma once

#include "platform_config.h"

#ifdef _WIN32
#include <WinSock2.h>
#else
#include <sys/queue.h>
#endif


#ifdef H_LIBEVENT_VERSION_14
#include <event.h>
#include <evhttp.h>
#include <evutil.h>
#include <evdns.h>
#else
#include <event2/event.h>
#include <event2/event_struct.h>
#include <event2/buffer.h>
#include <event2/bufferevent.h>
#include <event2/http.h>
#include <event2/http_compat.h>
#include <event2/http_struct.h>
#include <event2/event_compat.h>
#include <event2/dns.h>
#include <event2/dns_compat.h>
#include <event2/dns_struct.h>
#include <event2/listener.h>
#ifdef _DEBUG
#if LIBEVENT_VERSION_NUMBER >= 0x02010500
#define  ev_arg ev_evcallback.evcb_arg
#endif // LIBEVENT_VERSION_NUMBER
#endif // _DEBUG
#endif // H_LIBEVENT_VERSION_14

#ifndef evtimer_new
#define evtimer_new(b, cb, arg)        event_new((b), -1, 0, (cb), (arg))
#endif

#ifdef H_LIBEVENT_VERSION_14
extern "C" {
    struct evdns_base;
    EVPP_EXPORT struct event* event_new(struct event_base* base, evpp_socket_t fd, short events, void(*cb)(int, short, void*), void* arg);
    EVPP_EXPORT void event_free(struct event* ev);
    EVPP_EXPORT evhttp_connection* evhttp_connection_base_new(
        struct event_base* base, struct evdns_base* dnsbase,
        const char* address, unsigned short port);
}

// There is a bug of event timer for libevent1.4 on windows platform.
//   libevent1.4 use '#define evtimer_set(ev, cb, arg)  event_set(ev, -1, 0, cb, arg)' to assign a timer,
//   but '#define event_initialized(ev) ((ev)->ev_flags & EVLIST_INIT && (ev)->ev_fd != (int)INVALID_HANDLE_VALUE)'
//   So, if we use a event timer on windows, event_initialized(ev) will never return true.
#ifdef event_initialized
#undef event_initialized
#endif
#define event_initialized(ev) ((ev)->ev_flags & EVLIST_INIT)

#endif

