#pragma once

#ifdef _DEBUG
#ifndef H_INTERNAL_STATS
#define H_INTERNAL_STATS
#endif
#endif

#include <atomic>
#include "evpp/duration.h"

namespace evpp {
namespace http {
namespace stats {

// 这三个时间相加就是一个请求在应用层正真耗费的处理时间
struct Time {
    Duration dispatched_time; // 从接收到一个请求开始计时，到该请求被调度到工作线程开始执行，之间的消耗的时间
    Duration execute_time; // 该请求在工作线程中执行过程耗费的时间
    Duration response_time; // 该请求在工作线程执行完成时开始计时，到该请求调度到监听线程完成发送工作为止，之间消耗的时间
};

struct Count {
    std::atomic<uint64_t> recv; // 接收到的请求个数
    std::atomic<uint64_t> dispatched; // 分发到工作线程中的请求个数
    std::atomic<uint64_t> responsed; // 给客户端回应的请求个数
    std::atomic<uint64_t> failed; // 处理失败的请求个数
    std::atomic<uint64_t> slow; // 慢请求个数（处理时间超过一定的阈值）
};
}
}
}