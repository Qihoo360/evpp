#include <evpp/logging.h>

#include "test_common.h"

TEST_UNIT(testglog) {
    google::InitGoogleLogging("xxx");
    FLAGS_stderrthreshold = 0;
    LOG(INFO) << "INFO";
    LOG(WARNING) << "WARNING";
    LOG(ERROR) << "ERROR";
}