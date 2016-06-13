#pragma once

#include "inner_pre.h"

#include "duration.h"

#ifdef H_OS_WINDOWS
#include <functional>
#else
#include <tr1/functional>
#endif

struct event;
struct event_base;

namespace evpp {


    class EVPP_EXPORT EventWatcher {
    public:
        typedef xstd::function<void()> Handler;

        virtual ~EventWatcher();

        bool Init();

        // @note It MUST be called in the event thread.
        void Cancel();

    public:
        //! @brief : set_cancel_callback 
        //! @param[IN] const Handler & cb - The callback which will be called when this event is canceled.
        //! @return void - 
        void set_cancel_callback(const Handler& cb);

    protected:
        // @note It MUST be called in the event thread.
        // @param timeout the maximum amount of time to wait for the event, or 0 to wait forever
        bool Watch(Duration timeout);

        void set_evbase(struct event_base* evbase) {
            evbase_ = evbase;
        }

    protected:
        EventWatcher(struct event_base* evbase, const Handler& handler);

        void Close();

        virtual bool DoInit() = 0;
        virtual void DoClose() {}

    protected:
        static void HandleEvent(int, short, void*);

        struct event*      event_;
        struct event_base* evbase_;
        Handler handler_;
        Handler cancel_callback_;
    };

    //////////////////////////////////////////////////////////////////////////
    class EVPP_EXPORT PipeEventWatcher : public EventWatcher {
    public:
        PipeEventWatcher(struct event_base *event_base,
            const Handler& handler);

        void AsyncWait();
        void Notify();
    private:
        virtual bool DoInit();
        virtual void DoClose();
        static void HandlerFn(int fd, short which, void *v);

        int pipe_[2]; // Write to pipe_[0] , Read from pipe_[1]
    };

    //////////////////////////////////////////////////////////////////////////
    class EVPP_EXPORT TimerEventWatcher : public EventWatcher {
    public:
        TimerEventWatcher(struct event_base *event_base, const Handler& handler, Duration timeout);

        bool AsyncWait();

    private:
        virtual bool DoInit();
        static void HandlerFn(int fd, short which, void *v);
    private:
        Duration timeout_;
    };

    typedef TimerEventWatcher EventTimer;

    //////////////////////////////////////////////////////////////////////////
#ifndef H_OS_WINDOWS
    class SignalEventWatcher : public EventWatcher {
    public:
        SignalEventWatcher(int signo, struct event_base *event_base,
            const Handler& handler);

    private:
        virtual bool DoInit();
        static void HandlerFn(int sn, short which, void *v);

        int signo_;
    };
#endif
} // namespace 

