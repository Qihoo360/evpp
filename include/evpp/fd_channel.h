#pragma once

#include "libevent_watcher.h"

#include "evpp/base/timestamp.h"

struct event;
struct event_base;

namespace evpp {

    //
    // A selectable I/O channel.
    //
    // This class doesn't own the file descriptor.
    // The file descriptor could be a socket,
    // an eventfd, a timerfd, or a signalfd
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

        // Add this FdChannel to EventLoop
        bool Start();

        void Close();

    public:
        int fd() const { return fd_; }

    public:
        void SetReadCallback(const ReadEventCallback& cb) {
            read_fn_ = cb;
        }

        void SetWriteCallback(const EventCallback& cb) {
            write_fn_ = cb;
        }

        void SetCloseCallback(const EventCallback& cb) {
            close_fn_ = cb;
        }

        void SetErrorCallback(const EventCallback& cb) {
            error_fn_ = cb;
        }
    private:
        void HandlerFn(int fd, short which);
        static void HandlerFn(int fd, short which, void *v);

    private:
        ReadEventCallback read_fn_;
        EventCallback write_fn_;
        EventCallback close_fn_;
        EventCallback error_fn_;

        struct event*      event_;
        struct event_base* evbase_;

        int events_;
        int fd_;
    };

} // namespace 


