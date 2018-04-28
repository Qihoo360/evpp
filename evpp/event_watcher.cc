#include "evpp/inner_pre.h"

#include <string.h>

#include "evpp/libevent.h"
#include "evpp/event_watcher.h"
#include "evpp/event_loop.h"
#include "evpp/logging.h"

namespace evpp {

EventWatcher::EventWatcher(struct event_base* evbase, const Handler& handler)
    : evbase_(evbase), attached_(false), handler_(handler) {
    event_ = new event;
    memset(event_, 0, sizeof(struct event));
}

EventWatcher::EventWatcher(struct event_base* evbase, Handler&& handler)
    : evbase_(evbase), attached_(false), handler_(std::move(handler)) {
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

bool EventWatcher::Watch(Duration timeout) {
    struct timeval tv;
    struct timeval* timeoutval = nullptr;
    if (timeout.Nanoseconds() > 0) {
        timeout.To(&tv);
        timeoutval = &tv;
    }

    if (attached_) {
        // When InvokerTimer::periodic_ == true, EventWatcher::Watch will be called many times
        // so we need to remove it from event_base before we add it into event_base
        if (EventDel(event_) != 0) {
            LOG_ERROR << "event_del failed. fd=" << this->event_->ev_fd << " event_=" << event_;
            // TODO how to deal with it when failed?
        }
        attached_ = false;
    }

    assert(!attached_);
    if (EventAdd(event_, timeoutval) != 0) {
        LOG_ERROR << "event_add failed. fd=" << this->event_->ev_fd << " event_=" << event_;
        return false;
    }
    attached_ = true;
    return true;
}

void EventWatcher::FreeEvent() {
    if (event_) {
        if (attached_) {
            EventDel(event_);
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

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

PipeEventWatcher::PipeEventWatcher(EventLoop* loop,
                                   const Handler& handler)
    : EventWatcher(loop->event_base(), handler) {
    memset(pipe_, 0, sizeof(pipe_[0]) * 2);
}

PipeEventWatcher::PipeEventWatcher(EventLoop* loop,
                                   Handler&& h)
    : EventWatcher(loop->event_base(), std::move(h)) {
    memset(pipe_, 0, sizeof(pipe_[0]) * 2);
}


PipeEventWatcher::~PipeEventWatcher() {
    Close();
}

bool PipeEventWatcher::DoInit() {
    assert(pipe_[0] == 0);

    if (evutil_socketpair(AF_UNIX, SOCK_STREAM, 0, pipe_) < 0) {
        int err = errno;
        LOG_ERROR << "create socketpair ERROR errno=" << err << " " << strerror(err);
        goto failed;
    }

    if (evutil_make_socket_nonblocking(pipe_[0]) < 0 ||
        evutil_make_socket_nonblocking(pipe_[1]) < 0) {
        goto failed;
    }

    ::event_set(event_, pipe_[1], EV_READ | EV_PERSIST,
                &PipeEventWatcher::HandlerFn, this);
    return true;
failed:
    Close();
    return false;
}

void PipeEventWatcher::DoClose() {
    if (pipe_[0] > 0) {
        EVUTIL_CLOSESOCKET(pipe_[0]);
        EVUTIL_CLOSESOCKET(pipe_[1]);
        memset(pipe_, 0, sizeof(pipe_[0]) * 2);
    }
}

void PipeEventWatcher::HandlerFn(evpp_socket_t fd, short /*which*/, void* v) {
    LOG_INFO << "PipeEventWatcher::HandlerFn fd=" << fd << " v=" << v;
    PipeEventWatcher* e = (PipeEventWatcher*)v;
#ifdef H_BENCHMARK_TESTING
    // Every time we only read 1 byte for testing the IO event performance.
    // We use it in the benchmark test program 
    //  1. evpp/benchmark/ioevent/evpp/
    //  1. evpp/benchmark/ioevent/fd_channel_vs_pipe_event_watcher/
    char buf[1];
#else
    char buf[128];
#endif
    int n = 0;

    if ((n = ::recv(e->pipe_[1], buf, sizeof(buf), 0)) > 0) {
        e->handler_();
    }
}

bool PipeEventWatcher::AsyncWait() {
    return Watch(Duration());
}

void PipeEventWatcher::Notify() {
    char buf[1] = {};

    if (::send(pipe_[0], buf, sizeof(buf), 0) < 0) {
        return;
    }
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
TimerEventWatcher::TimerEventWatcher(EventLoop* loop,
                                     const Handler& handler,
                                     Duration timeout)
    : EventWatcher(loop->event_base(), handler)
    , timeout_(timeout) {}

TimerEventWatcher::TimerEventWatcher(EventLoop* loop,
                                     Handler&& h,
                                     Duration timeout)
    : EventWatcher(loop->event_base(), std::move(h))
    , timeout_(timeout) {}

TimerEventWatcher::TimerEventWatcher(struct event_base* loop,
                                     const Handler& handler,
                                     Duration timeout)
    : EventWatcher(loop, handler)
    , timeout_(timeout) {}

TimerEventWatcher::TimerEventWatcher(struct event_base* loop,
                                     Handler&& h,
                                     Duration timeout)
    : EventWatcher(loop, std::move(h))
    , timeout_(timeout) {}

bool TimerEventWatcher::DoInit() {
    ::event_set(event_, -1, 0, TimerEventWatcher::HandlerFn, this);
    return true;
}

void TimerEventWatcher::HandlerFn(evpp_socket_t /*fd*/, short /*which*/, void* v) {
    TimerEventWatcher* h = (TimerEventWatcher*)v;
    h->handler_();
}

bool TimerEventWatcher::AsyncWait() {
    return Watch(timeout_);
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
SignalEventWatcher::SignalEventWatcher(signal_number_t signo, EventLoop* loop,
                                       const Handler& handler)
    : EventWatcher(loop->event_base(), handler)
    , signo_(signo) {
    assert(signo_);
}

SignalEventWatcher::SignalEventWatcher(signal_number_t signo, EventLoop* loop,
                                       Handler&& h)
    : EventWatcher(loop->event_base(), std::move(h))
    , signo_(signo) {
    assert(signo_);
}

bool SignalEventWatcher::DoInit() {
    assert(signo_);
    signal_set(event_, signo_, SignalEventWatcher::HandlerFn, this);
    return true;
}

void SignalEventWatcher::HandlerFn(signal_number_t /*sn*/, short /*which*/, void* v) {
    SignalEventWatcher* h = (SignalEventWatcher*)v;
    h->handler_();
}

bool SignalEventWatcher::AsyncWait() {
    return Watch(Duration());
}
}

