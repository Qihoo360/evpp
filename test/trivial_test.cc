#include "test_common.h"

#include <evpp/sockets.h>
#include <evpp/logging.h>

TEST_UNIT(Teststrerror) {
    LOG_ERROR << evpp::strerror(EAGAIN);
}



