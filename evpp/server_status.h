#pragma once

#include <atomic>

#include "evpp/inner_pre.h"

namespace evpp {
class ServerStatus {
public:
    enum Status {
        kNull = 0,
        kInitialized = 1,
        kStarting = 2,
        kRunning = 3,
        kStopping = 4,
        kStopped = 5,
    };

    std::string ToString() const {
        H_CASE_STRING_BIGIN(status_);
        H_CASE_STRING(kNull);
        H_CASE_STRING(kInitialized);
        H_CASE_STRING(kRunning);
        H_CASE_STRING(kStopping);
        H_CASE_STRING(kStopped);
        H_CASE_STRING_END();
    }

protected:
    std::atomic<int> status_ = { kNull };
};
}