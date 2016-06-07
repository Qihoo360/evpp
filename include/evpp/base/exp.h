#pragma once

#include "platform_config.h"
#include "evppbase_export.h"

#ifdef H_OS_WINDOWS
//! Define import lib macro. Add it in ANY CPP file in target host module.
H_LINK_LIB("libevppbase")
#endif // end of #ifdef H_OS_WINDOWS

