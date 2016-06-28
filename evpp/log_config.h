#pragma once

#include "evpp/platform_config.h"

#ifdef __cplusplus
    #define GOOGLE_GLOG_DLL_DECL           // ʹ�þ�̬glog��ʱ�����붨�����
    #define GLOG_NO_ABBREVIATED_SEVERITIES // û�����������,��˵��Ϊ��Windows.h��ͻ

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
