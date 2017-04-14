# evpp设计细节系列(1)：实现一个自管理的定时器


[https://github.com/Qihoo360/evpp]项目中有一个`InvokeTimer`对象，详细代码请参见[https://github.com/Qihoo360/evpp/blob/master/evpp/invoke_timer.h]。它是一个能自我管理定时器类，它可以将一个仿函数绑定到该定时器上，然后让该定时器自己管理并在预期的一段时间后执行该仿函数。

定时器原型声明可能是下面的样子：

```C++
class InvokeTimer {
public:
    InvokeTimer(struct event_base* evloop, double timeout_ms, const std::function<void()>& f);
    ~InvokeTimer();
    void Start();
};
```

这个是最基本的接口，可以设置一个仿函数，并设置一个过期时间，然后绑定到一个`event_base`对象上，然后就可以期待过了一个预期的时间后，我们设置的仿函数被调用了。

为了便于说明后续的多个版本的实现，我们先将基础的不变的代码先说明一下。

基础代码，我们采用[evpp]项目中的`TimerEventWatcher`，详细实现在这里[event_watcher.h]和[event_watcher.cc]

头文件`event_watcher.h`定义如下：

```C++
#pragma once

#include <functional>

struct event;
struct event_base;

namespace recipes {

class EventWatcher {
public:
    typedef std::function<void()> Handler;
    virtual ~EventWatcher();
    bool Init();
    void Cancel();

    void SetCancelCallback(const Handler& cb);
    void ClearHandler() { handler_ = Handler(); }
protected:
    EventWatcher(struct event_base* evbase, const Handler& handler);
    bool Watch(double timeout_ms);
    void Close();
    void FreeEvent();

    virtual bool DoInit() = 0;
    virtual void DoClose() {}

protected:
    struct event* event_;
    struct event_base* evbase_;
    bool attached_;
    Handler handler_;
    Handler cancel_callback_;
};

class TimerEventWatcher : public EventWatcher {
public:
    TimerEventWatcher(struct event_base* evbase, const Handler& handler, double timeout_ms);

    bool AsyncWait();

private:
    virtual bool DoInit();
    static void HandlerFn(int fd, short which, void* v);
private:
    double timeout_ms_;
};

}

```

实现文件`event_watcher.cc`如下：

```C++
#include <string.h>
#include <assert.h>

#include <event2/event.h>
#include <event2/event_struct.h>
#include <event2/event_compat.h>

#include <iostream>

#include "event_watcher.h"

namespace recipes {

EventWatcher::EventWatcher(struct event_base* evbase, const Handler& handler)
    : evbase_(evbase), attached_(false), handler_(handler) {
    event_ = new event;
    memset(event_, 0, sizeof(struct event));
}

EventWatcher::~EventWatcher() {
    FreeEvent();
    Close();
}

bool EventWatcher::Init() {
    if (!DoInit()) {
        goto failed;
    }

    ::event_base_set(evbase_, event_);
    return true;

failed:
    Close();
    return false;
}


void EventWatcher::Close() {
    DoClose();
}

bool EventWatcher::Watch(double timeout_ms) {
    struct timeval tv;
    struct timeval* timeoutval = nullptr;
    if (timeout_ms > 0) {
        tv.tv_sec = long(timeout_ms / 1000);
        tv.tv_usec = long(timeout_ms * 1000.0) % 1000;
        timeoutval = &tv;
    }

    if (attached_) {
        // When InvokerTimer::periodic_ == true, EventWatcher::Watch will be called many times
        // so we need to remove it from event_base before we add it into event_base
        if (event_del(event_) != 0) {
            std::cerr << "event_del failed. fd=" << this->event_->ev_fd << " event_=" << event_ << std::endl;
            // TODO how to deal with it when failed?
        }
        attached_ = false;
    }

    assert(!attached_);
    if (event_add(event_, timeoutval) != 0) {
        std::cerr << "event_add failed. fd=" << this->event_->ev_fd << " event_=" << event_ << std::endl;
        return false;
    }
    attached_ = true;
    return true;
}

void EventWatcher::FreeEvent() {
    if (event_) {
        if (attached_) {
            event_del(event_);
            attached_ = false;
        }

        delete (event_);
        event_ = nullptr;
    }
}

void EventWatcher::Cancel() {
    assert(event_);
    FreeEvent();

    if (cancel_callback_) {
        cancel_callback_();
        cancel_callback_ = Handler();
    }
}

void EventWatcher::SetCancelCallback(const Handler& cb) {
    cancel_callback_ = cb;
}


TimerEventWatcher::TimerEventWatcher(struct event_base* evbase,
                                     const Handler& handler,
                                     double timeout_ms)
    : EventWatcher(evbase, handler)
    , timeout_ms_(timeout_ms) {}

bool TimerEventWatcher::DoInit() {
    ::event_set(event_, -1, 0, TimerEventWatcher::HandlerFn, this);
    return true;
}

void TimerEventWatcher::HandlerFn(int /*fd*/, short /*which*/, void* v) {
    TimerEventWatcher* h = (TimerEventWatcher*)v;
    h->handler_();
}

bool TimerEventWatcher::AsyncWait() {
    return Watch(timeout_ms_);
}

}

```


### 一个最基本的实现：basic-01

仅仅是能够实现功能

### 能够实现最基本自我管理：basic-02

### 实现一个周期性的定时器

### 如果要取消怎么办

### 最终版本


[gtest]:https://github.com/google/googletest
[glog]:https://github.com/google/glog
[Golang]:https://golang.org
[muduo]:https://github.com/chenshuo/muduo
[libevent]:https://github.com/libevent/libevent
[libevent2]:https://github.com/libevent/libevent
[LevelDB]:https://github.com/google/leveldb
[rapidjson]:https://github.com/miloyip/
[Boost.Asio]:http://www.boost.org/
[boost.asio]:http://www.boost.org/
[asio]:http://www.boost.org/
[boost]:http://www.boost.org/
[evpp]:https://github.com/Qihoo360/evpp
[https://github.com/Qihoo360/evpp]:https://github.com/Qihoo360/evpp
[evmc]:https://github.com/Qihoo360/evpp/tree/master/apps/evmc
[evnsq]:https://github.com/Qihoo360/evpp/tree/master/apps/evnsq
[https://github.com/Qihoo360/evpp/blob/master/evpp/invoke_timer.h]:https://github.com/Qihoo360/evpp/blob/master/evpp/invoke_timer.h
[https://github.com/Qihoo360/evpp/blob/master/evpp/event_watcher.h]:https://github.com/Qihoo360/evpp/blob/master/evpp/event_watcher.h
[https://github.com/Qihoo360/evpp/blob/master/evpp/event_watcher.cc]:https://github.com/Qihoo360/evpp/blob/master/evpp/event_watcher.cc
[event_watcher.h]:https://github.com/Qihoo360/evpp/blob/master/evpp/event_watcher.h
[event_watcher.cc]:https://github.com/Qihoo360/evpp/blob/master/evpp/event_watcher.cc