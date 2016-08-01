#pragma once

#include "platform_config.h"
#include "evpp_export.h"
#include "sys_addrinfo.h"
#include "sys_sockets.h"
#include "sockets.h"
#include "log_config.h"

#ifndef H_CASE_STRING_BIGIN
#define H_CASE_STRING_BIGIN(state) switch(state){
#define H_CASE_STRING(state) case state:return #state;break;
#define H_CASE_STRING_END()  default:return "Unknown";break;}
#endif

struct event;
namespace evpp {
int EventAdd(struct event* ev, const struct timeval* timeout);
int EventDel(struct event*);
EVPP_EXPORT int GetActiveEventCount();
}
