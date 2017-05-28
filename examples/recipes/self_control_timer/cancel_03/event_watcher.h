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

    // @note It MUST be called in the event thread.
    void Cancel();

    // @brief :
    // @param[IN] const Handler& cb - The callback which will be called when this event is canceled.
    // @return void -
    void SetCancelCallback(const Handler& cb);

    void ClearHandler() { handler_ = Handler(); }
protected:
    // @note It MUST be called in the event thread.
    // @param timeout the maximum amount of time to wait for the event, or 0 to wait forever
    bool Watch(double timeout_ms);

protected:
    EventWatcher(struct event_base* evbase, const Handler& handler);

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

class PipeEventWatcher : public EventWatcher {
public:
    PipeEventWatcher(struct event_base* evbase, const Handler& handler);
    ~PipeEventWatcher();

    bool AsyncWait();
    void Notify();
private:
    virtual bool DoInit();
    virtual void DoClose();
    static void HandlerFn(evutil_socket_t fd, short which, void* v);

    evutil_socket_t pipe_[2]; // Write to pipe_[0] , Read from pipe_[1]
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

