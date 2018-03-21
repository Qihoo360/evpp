#pragma once

#include <glog/logging.h>
#include <memory>
#include <chrono>
#include <folly/io/async/HHWheelTimer.h>
#include <folly/io/async/EvppTimeoutManager.h>
#include <folly/io/async/AsyncTimeout.h>
using namespace folly;
namespace evpp {

class EvppHHWheelTimer {
 public:
	 EvppHHWheelTimer(const std::shared_ptr<EventLoop> & evloop, const int tick_interval_ms = 10, const int default_timeout_ms = 10)
		 :tick_interval_ms_(tick_interval_ms), evloop_(evloop) {
			 timeout_manager_.reset(new EvppTimeoutManager(evloop));
			 wheel_ = std::move(HHWheelTimer::newTimer(timeout_manager_.get(), std::chrono::milliseconds(tick_interval_ms_), AsyncTimeout::InternalEnum::NORMAL, std::chrono::milliseconds(default_timeout_ms)));
	 }
     
	 //Return the number of currently pending timeouts
	 inline uint64_t count() const {
		 return wheel_->count();
	 }

	 std::size_t cancelAll() {
		 evloop_->RunInLoop([this](){this->wheel_->cancelAll();});
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
		 evloop_->RunInLoop([fn, timeout_ms, this]() {this->wheel_->scheduleTimeoutFn(fn, std::chrono::milliseconds(timeout_ms));});
	 }
 private:
	 int tick_interval_ms_;
	 HHWheelTimer::UniquePtr wheel_;
	 std::shared_ptr<EvppTimeoutManager> timeout_manager_;
	 std::shared_ptr<EventLoop> evloop_;
};
}
