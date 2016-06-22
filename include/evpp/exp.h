#pragma once

#include "platform_config.h"

#include "libevent_watcher.h"

#ifdef H_OS_WINDOWS
H_LINK_LIB("libevpp")
H_LINK_LIB("libevent")
H_LINK_LIB("libglog_static")
H_LINK_LIB("Ws2_32")
#endif




