#pragma once

#ifdef _WIN32
#ifndef EVMC_EXPORTS
#pragma comment(lib, "evmc_static.lib")
#endif
#endif

#include <evpp/platform_config.h>
#include <evpp/logging.h>