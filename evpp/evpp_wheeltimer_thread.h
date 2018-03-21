#pragma once

#include <glog/logging.h>
#include <memory>
#include <chrono>
#include <folly/io/async/HHWheelTimer.h>
#include <folly/io/async/AsyncTimeout.h>
#include <evpp/event_loop.h>
using namespace folly;
namespace evpp {

class EvppHHWheelTimer {
public:
    EvppHHWheelTimer(const std::shared_ptr<EventLoop>& evloop, const int tick_interval_ms = 10, const int default_timeout_ms = 10)
        : tick_interval_ms_(tick_interval_ms), evloop_(evloop) {
        wheel_ = std::move(HHWheelTimer::newTimer(evloop_.get(), std::chrono::milliseconds(tick_interval_ms_), AsyncTimeout::InternalEnum::NORMAL, std::chrono::milliseconds(default_timeout_ms)));
    }

    EvppHHWheelTimer(const int tick_interval_ms = 10, const int default_timeout_ms = 10)
        : tick_interval_ms_(tick_interval_ms) {
        evloop_.reset(new EventLoop());
        assert(evloop_);
        thr_ = new std::thread([evloop = evloop_]() {
            evloop->Run();
        });
        assert(thr_);
        evloop_->WaitUntilRunning();
        wheel_ = std::move(HHWheelTimer::newTimer(evloop_.get(), std::chrono::milliseconds(tick_interval_ms_), AsyncTimeout::InternalEnum::NORMAL, std::chrono::milliseconds(default_timeout_ms)));
    }
    EvppHHWheelTimer(const EvppHHWheelTimer& t) = delete;
    EvppHHWheelTimer& operator=(const EvppHHWheelTimer& t) = delete;
    EvppHHWheelTimer(EvppHHWheelTimer&& t) = delete;


    ~EvppHHWheelTimer() {
        cancelAll();
        if (thr_) {
            evloop_->Stop();
            thr_->join();
            delete thr_;
        }
    }

    //Return the number of currently pending timeouts
    inline uint64_t count() const {
        return wheel_->count();
    }

    std::size_t cancelAll() {
        evloop_->RunInLoop([this]() {
            this->wheel_->cancelAll();
        });
        return 0;
    }

    inline bool isDetachable() const {
        return wheel_->isDetachable();
    }

    inline int getTickInterval() const {
        return wheel_->getTickInterval().count();
    }

    inline int getDefaultTimeout() const {
        return wheel_->getDefaultTimeout().count();
    }

    inline void scheduleTimeout(const std::function<void()>& fn, int timeout_ms) {
        evloop_->RunInLoop([fn, timeout_ms, this]() {
            this->wheel_->scheduleTimeoutFn(fn, std::chrono::milliseconds(timeout_ms));
        });
    }
    static EvppHHWheelTimer* Instance() {
        static EvppHHWheelTimer* instance = new EvppHHWheelTimer();
        return instance;
    }

private:
    const int tick_interval_ms_;
    HHWheelTimer::UniquePtr wheel_;
    //HHWheelTimer* wheel_;
    std::thread*  thr_{nullptr};
    std::shared_ptr<EventLoop> evloop_;
};
}
