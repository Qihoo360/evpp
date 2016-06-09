#pragma once

#include "libevent_watcher.h"

#include "evpp/base/timestamp.h"

struct event;
struct event_base;

namespace evpp {

    class EventLoop;

    // A selectable I/O channel.
    //
    // This class doesn't own the file descriptor.
    // The file descriptor could be a socket,
    // an eventfd, a timerfd, or a signalfd
    class EVPP_EXPORT FdChannel {
    public:
        enum EventType {
            kNone = 0x00,
            kReadable = 0x02,
            kWritable = 0x04,
        };
        typedef xstd::function<void()> EventCallback;
        typedef xstd::function<void(base::Timestamp)> ReadEventCallback;

    public:
        FdChannel(EventLoop* loop, int fd,
                  bool watch_read_event, bool watch_write_event);

        void Close();

        // Attach this FdChannel to EventLoop
        void AttachToLoop();

    public:
        bool IsReadable() const { return (events_ & kReadable) != 0; }
        bool IsWritable() const { return (events_ & kWritable) != 0; }
        bool IsNoneEvent() const { return events_ == kNone; }

        void EnableReadEvent() { events_ |= kReadable; Update(); }
        void EnableWriteEvent() { events_ |= kWritable; Update(); }
        void DisableReadEvent() { events_ &= ~kReadable; Update(); }
        void DisableWriteEvent() { events_ &= ~kWritable; Update(); }
        void DisableAllEvent() { events_ = kNone; Update(); }

    public:
        int fd() const { return fd_; }
        std::string EventsToString() const;

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
        void HandleEvent(int fd, short which);
        static void HandleEvent(int fd, short which, void *v);

        void Update();
    private:
        ReadEventCallback read_fn_;
        EventCallback write_fn_;
        EventCallback close_fn_;
        EventCallback error_fn_;

        EventLoop* loop_;
        bool attached_to_loop_;

        struct event* event_;
        int events_; // the bitwise OR of zero or more of the EventType flags
        
        int fd_;
    };

} // namespace 


