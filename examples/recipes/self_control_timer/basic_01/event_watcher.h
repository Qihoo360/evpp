#pragma once

#include <event2/event.h>
#include <event2/event_struct.h>
#include <event2/event_compat.h>

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
    static void HandlerFn(evutil_socket_t fd, short which, void* v);
private:
    double timeout_ms_;
};

}

