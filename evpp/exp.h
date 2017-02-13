#pragma once

#include "platform_config.h"

#ifdef H_OS_WINDOWS
H_LINK_LIB("evpp")
H_LINK_LIB("libglog_static")
H_LINK_LIB("Ws2_32")
H_LINK_LIB("event")
H_LINK_LIB("event_core") // libevent2.0
H_LINK_LIB("event_extra") // libevent2.0
#endif




