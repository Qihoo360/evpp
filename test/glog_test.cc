#include <evpp/exp.h>
#include "test_common.h"

#ifdef H_WINDOWS_API
#	pragma comment(lib,"libglog_static.lib")
#endif

TEST_UNIT(testglog)
{
    google::InitGoogleLogging("xxx");
    FLAGS_stderrthreshold = 0;
    LOG(INFO) << "INFO";
    LOG(WARNING) << "WARNING";
    LOG(ERROR) << "ERROR";
}