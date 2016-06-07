#pragma once

#include "libevent_watcher.h"

#include "evpp/base/timestamp.h"

struct event;
struct event_base;

namespace evpp {
    class EVPP_EXPORT FdChannel {
    public:
        enum EventType {
            kNone     = 0x00,
            kReadable = 0x02,
            kWritable = 0x04,
        };
        typedef xstd::function<void()> EventCallback;
        typedef xstd::function<void(base::Timestamp)> ReadEventCallback;

    public:
        FdChannel(struct event_base *evbase, int fd,
            bool watch_read_event, bool watch_write_event);

        bool Start();
        void Close();

        void set_read_callback(const ReadEventCallback& cb) {
            read_fn_ = cb;
        }

        void set_write_callback(const EventCallback& cb) {
            write_fn_ = cb;
        }

        int fd() const { return fd_; }
    private:
        void HandlerFn(int fd, short which);
        static void HandlerFn(int fd, short which, void *v);

    private:
        ReadEventCallback read_fn_;
        EventCallback write_fn_;

        struct event*      event_;
        struct event_base* evbase_;

        int events_;
        int fd_;
    };

} // namespace 


