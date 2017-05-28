#include <string.h>
#include <assert.h>

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

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

PipeEventWatcher::PipeEventWatcher(struct event_base* evbase,
                                   const Handler& handler)
    : EventWatcher(evbase, handler) {
    memset(pipe_, 0, sizeof(pipe_[0] * 2));
}

PipeEventWatcher::~PipeEventWatcher() {
    Close();
}

bool PipeEventWatcher::DoInit() {
    assert(pipe_[0] == 0);

    if (evutil_socketpair(AF_UNIX, SOCK_STREAM, 0, pipe_) < 0) {
        int err = errno;
        std::cerr << "create socketpair ERROR errno=" << err << " " << strerror(err) << std::endl;
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

void PipeEventWatcher::HandlerFn(evutil_socket_t /*fd*/, short /*which*/, void* v) {
    PipeEventWatcher* e = (PipeEventWatcher*)v;
    char buf[128];
    int n = 0;
    if ((n = ::recv(e->pipe_[1], buf, sizeof(buf), 0)) > 0) {
        e->handler_();
    }
}

bool PipeEventWatcher::AsyncWait() {
    return Watch(0.0);
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
TimerEventWatcher::TimerEventWatcher(struct event_base* evbase,
                                     const Handler& handler,
                                     double timeout_ms)
    : EventWatcher(evbase, handler)
    , timeout_ms_(timeout_ms) {}

bool TimerEventWatcher::DoInit() {
    ::event_set(event_, -1, 0, TimerEventWatcher::HandlerFn, this);
    return true;
}

void TimerEventWatcher::HandlerFn(evutil_socket_t /*fd*/, short /*which*/, void* v) {
    TimerEventWatcher* h = (TimerEventWatcher*)v;
    h->handler_();
}

bool TimerEventWatcher::AsyncWait() {
    return Watch(timeout_ms_);
}

}

