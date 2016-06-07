#pragma once

#include "evpp/base/platform_config.h"

#ifdef __cplusplus
    #define GOOGLE_GLOG_DLL_DECL           // 使用静态glog库时，必须定义这个
    #define GLOG_NO_ABBREVIATED_SEVERITIES // 没这个编译会出错,传说因为和Windows.h冲突

    #include <glog/logging.h>

    #ifdef GOOGLE_LOG_INFO
        #define LOG_TRACE LOG(INFO)
        #define LOG_DEBUG LOG(INFO)
        #define LOG_INFO  LOG(INFO)
        #define LOG_WARN  LOG(WARNING)
        #define LOG_ERROR LOG(ERROR)
        #define LOG_FATAL LOG(FATAL)
    #else
        #define LOG_TRACE std::cout << __FILE__ << ":" << __LINE__
        #define LOG_DEBUG std::cout << __FILE__ << ":" << __LINE__
        #define LOG_INFO  std::cout << __FILE__ << ":" << __LINE__
        #define LOG_WARN  std::cout << __FILE__ << ":" << __LINE__
        #define LOG_ERROR std::cout << __FILE__ << ":" << __LINE__
        #define LOG_FATAL std::cout << __FILE__ << ":" << __LINE__
        #define CHECK_NOTNULL(val) LOG_ERROR << "'" #val "' Must be non NULL";
    #endif
#endif // end of define __cplusplus
