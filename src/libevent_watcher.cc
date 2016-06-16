#include "evpp/inner_pre.h"

#include <string.h>

#include "evpp/libevent_watcher.h"
#include "evpp/libevent_headers.h"
#include "evpp/event_loop.h"

namespace evpp {

    EventWatcher::EventWatcher(struct event_base* evbase, const Handler& handler)
        : evbase_(evbase), handler_(handler) {
        event_ = new event;
        memset(event_, 0, sizeof(struct event));
    }

    EventWatcher::~EventWatcher() {
        Cancel();
        Close();
        if (event_) {
            delete (event_);
            event_ = 0;
        }
    }

    bool EventWatcher::Init() {
        if (!DoInit()) {
            goto failed;
        }
        event_base_set(evbase_, event_);
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
        struct timeval* timeoutval = NULL;

        if (timeout.Nanoseconds() > 0) {
            timeout.To(&tv);
            timeoutval = &tv;
        }

        if (event_add(event_, timeoutval) != 0) {
            return false;
        }

        return true;
    }

    void EventWatcher::Cancel() {
        if (event_) {
            if (event_initialized(event_)) {
                event_del(event_);
            }
            delete (event_);
            event_ = NULL;
        }

        if (cancel_callback_) {
            cancel_callback_();
        }
    }

    void EventWatcher::set_cancel_callback(const Handler& cb) {
        cancel_callback_ = cb;
    }


    //////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////

    PipeEventWatcher::PipeEventWatcher(struct event_base *event_base,
                                       const Handler& handler)
                                       : EventWatcher(event_base, handler) {
        memset(pipe_, 0, sizeof(pipe_[0] * 2));
    }

    PipeEventWatcher::PipeEventWatcher(EventLoop* loop,
                                       const Handler& handler)
                                       : EventWatcher(loop->event_base(), handler) {
        memset(pipe_, 0, sizeof(pipe_[0] * 2));
    }

    bool PipeEventWatcher::DoInit() {
        assert(pipe_[0] == 0);

        if (evutil_socketpair(AF_UNIX, SOCK_STREAM, 0, pipe_) < 0) {
            LOG_ERROR << "create socketpair ERROR";
            goto failed;
        }

        if (evutil_make_socket_nonblocking(pipe_[0]) < 0 ||
            evutil_make_socket_nonblocking(pipe_[1]) < 0) {
            goto failed;
        }

        event_set(event_, pipe_[1], EV_READ | EV_PERSIST,
                  PipeEventWatcher::HandlerFn, this);
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

    void PipeEventWatcher::HandlerFn(int fd, short which, void *v) {
        PipeEventWatcher *e = (PipeEventWatcher*)v;
        char buf[128];
        int n = 0;
        if ((n = ::recv(e->pipe_[1], buf, sizeof(buf), 0)) > 0) {
            e->handler_();
        }
    }

    void PipeEventWatcher::AsyncWait() {
        Watch(Duration());
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

    TimerEventWatcher::TimerEventWatcher(struct event_base *event_base,
                                         const Handler& handler,
                                         Duration timeout)
                                         : EventWatcher(event_base, handler)
                                         , timeout_(timeout) {}

    TimerEventWatcher::TimerEventWatcher(EventLoop* loop,
                                         const Handler& handler,
                                         Duration timeout)
                                         : EventWatcher(loop->event_base(), handler)
                                         , timeout_(timeout) {}

    bool TimerEventWatcher::DoInit() {
        event_set(event_, -1, 0, TimerEventWatcher::HandlerFn, this);
        return true;
    }

    void TimerEventWatcher::HandlerFn(int fd, short which, void *v) {
        TimerEventWatcher *h = (TimerEventWatcher*)v;
        h->handler_();
    }

    bool TimerEventWatcher::AsyncWait() {
        return Watch(timeout_);
    }

    //////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////
#ifndef H_OS_WINDOWS
    SignalEventWatcher::SignalEventWatcher(int signo, struct event_base *event_base,
                                           const Handler& handler)
                                           : EventWatcher(event_base, handler)
                                           , signo_(signo) {
        assert(signo_);
    }

    SignalEventWatcher::SignalEventWatcher(int signo, EventLoop* loop,
                                           const Handler& handler)
                                           : EventWatcher(loop->event_base(), handler)
                                           , signo_(signo) {
        assert(signo_);
    }

    bool SignalEventWatcher::DoInit() {
        assert(signo_);
        signal_set(event_, signo_, SignalEventWatcher::HandlerFn, this);
        return true;
    }

    void SignalEventWatcher::HandlerFn(int sn, short which, void *v) {
        SignalEventWatcher *h = (SignalEventWatcher*)v;
        h->handler_();
    }
#endif
}

