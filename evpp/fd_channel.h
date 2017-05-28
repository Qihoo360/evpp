#pragma once

#include "event_watcher.h"

struct event;
struct event_base;

namespace evpp {

class EventLoop;

// A selectable I/O fd channel.
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
    typedef std::function<void()> EventCallback;
    typedef std::function<void()> ReadEventCallback;

public:
    FdChannel(EventLoop* loop, evpp_socket_t fd,
              bool watch_read_event, bool watch_write_event);
    ~FdChannel();

    void Close();

    // Attach this FdChannel to EventLoop
    void AttachToLoop();

    bool attached() const {
        return attached_;
    }

public:
    bool IsReadable() const {
        return (events_ & kReadable) != 0;
    }
    bool IsWritable() const {
        return (events_ & kWritable) != 0;
    }
    bool IsNoneEvent() const {
        return events_ == kNone;
    }

    void EnableReadEvent();
    void EnableWriteEvent();
    void DisableReadEvent();
    void DisableWriteEvent();
    void DisableAllEvent();

public:
    evpp_socket_t fd() const {
        return fd_;
    }
    std::string EventsToString() const;

public:
    void SetReadCallback(const ReadEventCallback& cb) {
        read_fn_ = cb;
    }

    void SetWriteCallback(const EventCallback& cb) {
        write_fn_ = cb;
    }

private:
    void HandleEvent(evpp_socket_t fd, short which);
    static void HandleEvent(evpp_socket_t fd, short which, void* v);

    void Update();
    void DetachFromLoop();
private:
    ReadEventCallback read_fn_;
    EventCallback write_fn_;

    EventLoop* loop_;
    bool attached_; // A flag indicate whether this FdChannel has been attached to loop_

    struct event* event_;
    int events_; // the bitwise OR of zero or more of the EventType flags

    evpp_socket_t fd_;
};

}


