# evpp设计细节系列(1)：利用 enable_shared_from_this 实现一个自管理的定时器



# 0. 前言

[https://github.com/Qihoo360/evpp](https://github.com/Qihoo360/evpp)是一个高性能的Reactor模式的现代化的C++11版本的高性能网络库。该项目中有一个`InvokeTimer`对象，接口头文件详细代码请参见[https://github.com/Qihoo360/evpp/blob/master/evpp/invoke_timer.h](https://github.com/Qihoo360/evpp/blob/master/evpp/invoke_timer.h)。它是一个能自我管理的定时器类，可以将一个仿函数绑定到该定时器上，然后让该定时器自己管理并在预期的一段时间后执行该仿函数。

现在我们复盘一下这个功能的实现细节和演化过程。

# 1. 基础代码

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

为了便于说明后续的多个版本的实现，我们先将基础的不变的代码说明一下。

基础代码，我们采用[evpp]项目中的`TimerEventWatcher`，详细实现在这里[event_watcher.h]和[event_watcher.cc]。它是一个时间定时器观察者对象，可以观察一个时间事件。

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


# 2. 一个最基本的实现：basic-01

我们先尝试实现一个能满足最基本需求的定时器。

```C++

// 头文件
#include <memory>
#include <functional>

struct event_base;

namespace recipes {

class TimerEventWatcher;
class InvokeTimer;

class InvokeTimer {
public:
    typedef std::function<void()> Functor;

    InvokeTimer(struct event_base* evloop, double timeout_ms, const Functor& f);
    ~InvokeTimer();

    void Start();

private:
    void OnTimerTriggered();

private:
    struct event_base* loop_;
    double timeout_ms_;
    Functor functor_;
    std::shared_ptr<TimerEventWatcher> timer_;
};

}



// 实现文件
#include "invoke_timer.h"
#include "event_watcher.h"

#include <thread>
#include <iostream>

namespace recipes {

InvokeTimer::InvokeTimer(struct event_base* evloop, double timeout_ms, const Functor& f)
    : loop_(evloop), timeout_ms_(timeout_ms), functor_(f) {
    std::cout << "InvokeTimer::InvokeTimer tid=" << std::this_thread::get_id() << " this=" << this << std::endl;
}

InvokeTimer::~InvokeTimer() {
    std::cout << "InvokeTimer::~InvokeTimer tid=" << std::this_thread::get_id() << " this=" << this << std::endl;
}

void InvokeTimer::Start() {
    std::cout << "InvokeTimer::Start tid=" << std::this_thread::get_id() << " this=" << this << std::endl;
    timer_.reset(new TimerEventWatcher(loop_, std::bind(&InvokeTimer::OnTimerTriggered, this), timeout_ms_));
    timer_->Init();
    timer_->AsyncWait();
    std::cout << "InvokeTimer::Start(AsyncWait) tid=" << std::this_thread::get_id() << " timer=" << timer_.get() << " this=" << this << " timeout(ms)=" << timeout_ms_ << std::endl;
}

void InvokeTimer::OnTimerTriggered() {
    std::cout << "InvokeTimer::OnTimerTriggered tid=" << std::this_thread::get_id() << " this=" << this << std::endl;
    functor_();
    functor_ = Functor();
}

}


```

测试main.cc

```C++
#include "invoke_timer.h"
#include "event_watcher.h"
#include "winmain-inl.h"

#include <event2/event.h>

void Print() {
    std::cout << __FUNCTION__ << " hello world." << std::endl;
}

int main() {
    struct event_base* base = event_base_new();
    auto timer = new recipes::InvokeTimer(base, 1000.0, &Print);
    timer->Start();
    event_base_dispatch(base);
    event_base_free(base);
    delete timer;
    return 0;
}
```

我们先创建一个`event_base`对象，随后创建一个`InvokeTimer`对象，随后让timer启动起来，即将timer注册到`event_base`对象中，最后运行`event_base_dispatch(base)`。

下面编译运行，结果是符合预期的：当timer的时间到期后，能顺利触发回调。

```shell
$ ls -l
total 80
-rw-rw-r-- 1 weizili weizili  2729 Apr 15 20:39 event_watcher.cc
-rw-rw-r-- 1 weizili weizili   996 Apr 15 20:39 event_watcher.h
-rw-rw-r-- 1 weizili weizili  1204 Apr 14 10:55 invoke_timer.cc
-rw-rw-r-- 1 weizili weizili   805 Apr 14 10:55 invoke_timer.h
-rw-rw-r-- 1 weizili weizili   374 Apr 14 10:55 main.cc
$ g++ -std=c++11 event_watcher.cc invoke_timer.cc main.cc -levent
$ ./a.out 
InvokeTimer::InvokeTimer tid=139965845526336 this=0x7ffd2790f780
InvokeTimer::Start tid=139965845526336 this=0x7ffd2790f780
InvokeTimer::Start(AsyncWait) tid=139965845526336 timer=0x14504c0 this=0x7ffd2790f780 timeout(ms)=1000
InvokeTimer::OnTimerTriggered tid=139965845526336 this=0x7ffd2790f780
Print hello world.
InvokeTimer::~InvokeTimer tid=139965845526336 this=0x7ffd2790f780
```

这个实现方式，`InvokeTimer`对象生命周期的管理是一个问题，它需要调用者自己管理。


# 3. 能够实现最基本自我管理：basic-02

为了实现`InvokeTimer`对象生命周期的自我管理，其实就是调用者不需要关心`InvokeTimer`对象的生命周期问题。可以设想一下，假如`InvokeTimer`对象创建后，当定时时间一到，我们就调用其绑定的毁掉回函，然后`InvokeTimer`对象自我销毁，是不是就可以实现自我管理了呢？嗯，这个可行。请看下面代码。

```C++

// 头文件

#include <memory>
#include <functional>

struct event_base;

namespace recipes {

class TimerEventWatcher;
class InvokeTimer;

class InvokeTimer {
public:
    typedef std::function<void()> Functor;

    static InvokeTimer* Create(struct event_base* evloop,
                                 double timeout_ms,
                                 const Functor& f);

    ~InvokeTimer();

    void Start();

private:
    InvokeTimer(struct event_base* evloop, double timeout_ms, const Functor& f);
    void OnTimerTriggered();

private:
    struct event_base* loop_;
    double timeout_ms_;
    Functor functor_;
    std::shared_ptr<TimerEventWatcher> timer_;
};

}


// 实现文件

#include "invoke_timer.h"
#include "event_watcher.h"

#include <thread>
#include <iostream>

namespace recipes {

InvokeTimer::InvokeTimer(struct event_base* evloop, double timeout_ms, const Functor& f)
    : loop_(evloop), timeout_ms_(timeout_ms), functor_(f) {
    std::cout << "InvokeTimer::InvokeTimer tid=" << std::this_thread::get_id() << " this=" << this << std::endl;
}

InvokeTimer* InvokeTimer::Create(struct event_base* evloop, double timeout_ms, const Functor& f) {
    return new InvokeTimer(evloop, timeout_ms, f);
}

InvokeTimer::~InvokeTimer() {
    std::cout << "InvokeTimer::~InvokeTimer tid=" << std::this_thread::get_id() << " this=" << this << std::endl;
}

void InvokeTimer::Start() {
    std::cout << "InvokeTimer::Start tid=" << std::this_thread::get_id() << " this=" << this << std::endl;
    timer_.reset(new TimerEventWatcher(loop_, std::bind(&InvokeTimer::OnTimerTriggered, this), timeout_ms_));
    timer_->Init();
    timer_->AsyncWait();
    std::cout << "InvokeTimer::Start(AsyncWait) tid=" << std::this_thread::get_id() << " timer=" << timer_.get() << " this=" << this << " timeout(ms)=" << timeout_ms_ << std::endl;
}

void InvokeTimer::OnTimerTriggered() {
    std::cout << "InvokeTimer::OnTimerTriggered tid=" << std::this_thread::get_id() << " this=" << this << std::endl;
    functor_();
    functor_ = Functor();
    delete this;
}

}

```

请注意，上述实现中，为了实现自我销毁，我们必须调用 **delete** ，这就注定了`InvokeTimer`对象必须在堆上创建，因此我们隐藏了它的构造函数，然后用一个静态的 **Create** 成员来创建`InvokeTimer`对象的实例。

相应的，`main.cc`也做了一点点修改代码如下：


```C++
#include "invoke_timer.h"
#include "event_watcher.h"
#include "winmain-inl.h"

#include <event2/event.h>

void Print() {
    std::cout << __FUNCTION__ << " hello world." << std::endl;
}

int main() {
    struct event_base* base = event_base_new();
    auto timer = recipes::InvokeTimer::Create(base, 1000.0, &Print);
    timer->Start(); // 启动完成后，就不用关注该对象了
    event_base_dispatch(base);
    event_base_free(base);
    return 0;
}
```

这个实现，就不需要上层调用者手工`delete`这个`InvokeTimer`对象的实例，从而达到`InvokeTimer`对象自我管理的目的。

下面编译运行，结果是符合预期的：当timer时间到期后，能顺利触发回调，并且`InvokeTimer`对象也自动析构了。

```shell
$ ls -l
total 80
-rw-rw-r-- 1 weizili weizili  2729 Apr 15 20:39 event_watcher.cc
-rw-rw-r-- 1 weizili weizili   996 Apr 15 20:39 event_watcher.h
-rw-rw-r-- 1 weizili weizili  1204 Apr 14 10:55 invoke_timer.cc
-rw-rw-r-- 1 weizili weizili   805 Apr 14 10:55 invoke_timer.h
-rw-rw-r-- 1 weizili weizili   374 Apr 14 10:55 main.cc
$ g++ -std=c++11 event_watcher.cc invoke_timer.cc main.cc -levent
$ ./a.out 
InvokeTimer::InvokeTimer tid=139965845526336 this=0x7ffd2790f780
InvokeTimer::Start tid=139965845526336 this=0x7ffd2790f780
InvokeTimer::Start(AsyncWait) tid=139965845526336 timer=0x14504c0 this=0x7ffd2790f780 timeout(ms)=1000
InvokeTimer::OnTimerTriggered tid=139965845526336 this=0x7ffd2790f780
Print hello world.
InvokeTimer::~InvokeTimer tid=139965845526336 this=0x7ffd2790f780
```


# 4. 如果要取消一个定时器怎么办：cancel-03

上面第2种实现方式，实现了定时器的自我管理，调用者不需要关心定时器的生命周期的管理问题。接下来，新的需求又来了，上层调用者说，在对外发起一个请求时，可以设置一个定时器来处理超时问题，但如果请求及时的回来了，我们得及时取消该定时器啊，这又如何处理呢？

这就相当于要把上层调用者还得一直保留`InvokeTimer`对象的实例，以便在需要的时候，提前取消掉该定时器。上层调用者保留这个指针，就会带来一定的风险，例如误用，当`InvokeTimer`对象已经自动析构了，该该指针还继续存在于上层调用者那里。

于是乎，智能指针`shared_ptr`出场了，我们希望上层调用者看到的对象是以`shared_ptr<InvokeTimer>`形式存在的，无论上层调用者是否保留这个`shared_ptr<InvokeTimer>`对象，`InvokeTimer`对象都能自我管理，也就是说，当上层调用者不保留`shared_ptr<InvokeTimer>`对象时，`InvokeTimer`对象要能自我管理。

这里就必须让`InvokeTimer`对象本身也要保存一份`shared_ptr<InvokeTimer>`对象。为了实现这一技术，我们需要引入`enable_shared_from_this`。关于`enable_shared_from_this`的介绍，网络上已经有很多资料了，这里不多累述。我们直接上最终的实现代码：

```C++

// 头文件

#include <memory>
#include <functional>

struct event_base;

namespace recipes {

class TimerEventWatcher;
class InvokeTimer;

typedef std::shared_ptr<InvokeTimer> InvokeTimerPtr;

class InvokeTimer : public std::enable_shared_from_this<InvokeTimer> {
public:
    typedef std::function<void()> Functor;

    static InvokeTimerPtr Create(struct event_base* evloop,
                                 double timeout_ms,
                                 const Functor& f);

    ~InvokeTimer();

    void Start();

    void Cancel();

    void set_cancel_callback(const Functor& fn) {
        cancel_callback_ = fn;
    }
private:
    InvokeTimer(struct event_base* evloop, double timeout_ms, const Functor& f);
    void OnTimerTriggered();
    void OnCanceled();

private:
    struct event_base* loop_;
    double timeout_ms_;
    Functor functor_;
    Functor cancel_callback_;
    std::shared_ptr<TimerEventWatcher> timer_;
    std::shared_ptr<InvokeTimer> self_; // Hold myself
};

}


// 实现文件

#include "invoke_timer.h"
#include "event_watcher.h"

#include <thread>
#include <iostream>

namespace recipes {

InvokeTimer::InvokeTimer(struct event_base* evloop, double timeout_ms, const Functor& f)
    : loop_(evloop), timeout_ms_(timeout_ms), functor_(f) {
    std::cout << "InvokeTimer::InvokeTimer tid=" << std::this_thread::get_id() << " this=" << this << std::endl;
}

InvokeTimerPtr InvokeTimer::Create(struct event_base* evloop, double timeout_ms, const Functor& f) {
    InvokeTimerPtr it(new InvokeTimer(evloop, timeout_ms, f));
    it->self_ = it;
    return it;
}

InvokeTimer::~InvokeTimer() {
    std::cout << "InvokeTimer::~InvokeTimer tid=" << std::this_thread::get_id() << " this=" << this << std::endl;
}

void InvokeTimer::Start() {
    std::cout << "InvokeTimer::Start tid=" << std::this_thread::get_id() << " this=" << this << " refcount=" << self_.use_count() << std::endl;
    timer_.reset(new TimerEventWatcher(loop_, std::bind(&InvokeTimer::OnTimerTriggered, shared_from_this()), timeout_ms_));
    timer_->SetCancelCallback(std::bind(&InvokeTimer::OnCanceled, shared_from_this()));
    timer_->Init();
    timer_->AsyncWait();
    std::cout << "InvokeTimer::Start(AsyncWait) tid=" << std::this_thread::get_id() << " timer=" << timer_.get() << " this=" << this << " refcount=" << self_.use_count() << " periodic=" << periodic_ << " timeout(ms)=" << timeout_ms_ << std::endl;
}

void InvokeTimer::Cancel() {
    if (timer_) {
        timer_->Cancel();
    }
}

void InvokeTimer::OnTimerTriggered() {
    std::cout << "InvokeTimer::OnTimerTriggered tid=" << std::this_thread::get_id() << " this=" << this << " use_count=" << self_.use_count() << std::endl;
    functor_();
    functor_ = Functor();
    cancel_callback_ = Functor();
    timer_.reset();
    self_.reset();
}

void InvokeTimer::OnCanceled() {
    std::cout << "InvokeTimer::OnCanceled tid=" << std::this_thread::get_id() << " this=" << this << " use_count=" << self_.use_count() << std::endl;
    if (cancel_callback_) {
        cancel_callback_();
        cancel_callback_ = Functor();
    }
    functor_ = Functor();
    timer_.reset();
    self_.reset();
}

}


```



相应的，`main.cc`也做了一点点修改代码如下：


```C++
#include "invoke_timer.h"
#include "event_watcher.h"
#include "winmain-inl.h"

#include <event2/event.h>

void Print() {
    std::cout << __FUNCTION__ << " hello world." << std::endl;
}

int main() {
    struct event_base* base = event_base_new();
    auto timer = recipes::InvokeTimer::Create(base, 1000.0, &Print);
    timer->Start(); // 启动完成后，就不用关注该对象了
    event_base_dispatch(base);
    event_base_free(base);
    return 0;
}
```

这个实现，就不需要上层调用者手工`delete`这个`InvokeTimer`对象的实例，从而达到`InvokeTimer`对象自我管理的目的。

下面编译运行，结果是符合预期的：当timer时间到期后，能顺利触发回调，并且`InvokeTimer`对象也自动析构了。




# 5. 实现一个周期性的定时器：periodic-04

上述几个实现中，都是一次性的定时器任务。但是如果我们想实现一个周期性的定时器该如何实现呢？例如，我们有一个任务，需要每分钟做一次。

其实，基于上述第三个版本的实现，可以很容易的实现周期性的定时器功能。只需要在回调函数中，继续调用`timer->AsyncWait()`即可。详细的修改情况如下。


头文件 invoke_timer.h 改变：

```diff

@@ -18,7 +18,8 @@ public:

     static InvokeTimerPtr Create(struct event_base* evloop,
                                  double timeout_ms,
-                                 const Functor& f);
+                                 const Functor& f,
+                                 bool periodic);

     ~InvokeTimer();

@@ -30,7 +31,7 @@ public:
         cancel_callback_ = fn;
     }
 private:
-    InvokeTimer(struct event_base* evloop, double timeout_ms, const Functor& f);
+    InvokeTimer(struct event_base* evloop, double timeout_ms, const Functor& f, bool periodic);
     void OnTimerTriggered();
     void OnCanceled();

@@ -40,6 +41,7 @@ private:
     Functor functor_;
     Functor cancel_callback_;
     std::shared_ptr<TimerEventWatcher> timer_;
+    bool periodic_;
     std::shared_ptr<InvokeTimer> self_; // Hold myself
 };
```


实现文件 invoke_timer.cc 改变：

```diff

 namespace recipes {

-InvokeTimer::InvokeTimer(struct event_base* evloop, double timeout_ms, const Functor& f)
-    : loop_(evloop), timeout_ms_(timeout_ms), functor_(f) {
+InvokeTimer::InvokeTimer(struct event_base* evloop, double timeout_ms, const Functor& f, bool periodic)
+    : loop_(evloop), timeout_ms_(timeout_ms), functor_(f), periodic_(periodic) {
     std::cout << "InvokeTimer::InvokeTimer tid=" << std::this_thread::get_id() << " this=" << this << std::endl;
 }

-InvokeTimerPtr InvokeTimer::Create(struct event_base* evloop, double timeout_ms, const Functor& f) {
-    InvokeTimerPtr it(new InvokeTimer(evloop, timeout_ms, f));
+InvokeTimerPtr InvokeTimer::Create(struct event_base* evloop, double timeout_ms, const Functor& f, bool periodic) {
+    InvokeTimerPtr it(new InvokeTimer(evloop, timeout_ms, f, periodic));
     it->self_ = it;
     return it;
 }
@@ -27,7 +27,7 @@ void InvokeTimer::Start() {
     timer_->SetCancelCallback(std::bind(&InvokeTimer::OnCanceled, shared_from_this()));
     timer_->Init();
     timer_->AsyncWait();
 }

 void InvokeTimer::Cancel() {
@@ -39,14 +39,20 @@ void InvokeTimer::Cancel() {
 void InvokeTimer::OnTimerTriggered() {
     std::cout << "InvokeTimer::OnTimerTriggered tid=" << std::this_thread::get_id() << " this=" << this << " use_count=" << self_.use_count() << std::endl;
     functor_();
-    functor_ = Functor();
-    cancel_callback_ = Functor();
-    timer_.reset();
-    self_.reset();
+
+    if (periodic_) {
+        timer_->AsyncWait();
+    } else {
+        functor_ = Functor();
+        cancel_callback_ = Functor();
+        timer_.reset();
+        self_.reset();
+    }
 }

 void InvokeTimer::OnCanceled() {
     std::cout << "InvokeTimer::OnCanceled tid=" << std::this_thread::get_id() << " this=" << this << " use_count=" << self_.use_count() << std::endl;
+    periodic_ = false;
     if (cancel_callback_) {
         cancel_callback_();
         cancel_callback_ = Functor();

```

main.cc测试示例代码也有所修改，具体如下：


```C++
#include "invoke_timer.h"
#include "event_watcher.h"
#include "winmain-inl.h"

#include <event2/event.h>

void Print() {
    std::cout << __FUNCTION__ << " hello world." << std::endl;
}

int main() {
    struct event_base* base = event_base_new();
    auto timer = recipes::InvokeTimer::Create(base, 1000.0, &Print, true);
    timer->Start();
    timer.reset();
    event_base_dispatch(base);
    event_base_free(base);
    return 0;
}
```

该版本是最终的实现版本。相关代码都在[https://github.com/Qihoo360/evpp/tree/master/examples/recipes/self_control_timer]这里，为了便于演示，其不依赖[evpp]。


# 6. 最后

[evpp]项目官网地址为：[https://github.com/Qihoo360/evpp]
本文中的详细代码实现请参考 [https://github.com/Qihoo360/evpp/tree/master/examples/recipes/self_control_timer]

# 7. evpp系列文章列表

[evpp性能测试（3）: 对无锁队列boost::lockfree::queue和moodycamel::ConcurrentQueue做一个性能对比测试](http://blog.csdn.net/zieckey/article/details/69803011)
[evpp性能测试（2）: 与Boost.Asio进行吞吐量对比测试](http://blog.csdn.net/zieckey/article/details/69170619)
[evpp性能测试（1）: 与muduo进行吞吐量测试](http://blog.csdn.net/zieckey/article/details/63778715)
[发布一个高性能的Reactor模式的C++网络库：evpp](http://blog.csdn.net/zieckey/article/details/63760757)

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
[https://github.com/Qihoo360/evpp/tree/master/examples/recipes/self_control_timer]:https://github.com/Qihoo360/evpp/tree/master/examples/recipes/self_control_timer