#pragma once

#include <assert.h>
#include <stdint.h>

#ifdef __cplusplus
#include <iostream>
#include <memory>
#include <functional>
#endif // end of define __cplusplus

#include "platform_config.h"
#include "sys_addrinfo.h"
#include "sys_sockets.h"
#include "sockets.h"
#include "logging.h"

struct event;
namespace evpp {
    int EventAdd(struct event* ev, const struct timeval* timeout);
    int EventDel(struct event*);
    EVPP_EXPORT int GetActiveEventCount();
}
