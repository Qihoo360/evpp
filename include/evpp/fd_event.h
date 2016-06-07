#pragma once

#include "inner_pre.h"

struct event_base;
struct event;

namespace evpp {
    class EVPP_EXPORT FdEvent {
        static const int kTimedOut = 4;
    public:
        enum EventType {
            kReadable = 0x02,
            kWritable = 0x04,
        };
        typedef xstd::function<void(int /*enum EventType*/)> Functor;

        FdEvent(struct event_base* loop);
        ~FdEvent();

        //! brief : 
        //! param[in] - int fd
        //! param[in] - int events
        //! param[in] - const Functor & handler
        //! param[in] - uint32_t timeout_us - the maximum amount of time to wait for the event, 
        //!             or 0 to wait forever
        //! return - void
        void AsyncWait(int fd, int events/*enum EventType*/, const Functor& handler, uint32_t timeout_us = 0);
        void Cancel();

    protected:
        void Start(int fd, int events/*enum EventType*/, uint32_t timeout_us);
        static void Notify(int fd, short what/*enum EventType*/, void *arg);

        struct event_base* loop_;
        struct event *ev_;
        Functor handler_;
        int8_t flags_;
        bool active_;
    };
}
