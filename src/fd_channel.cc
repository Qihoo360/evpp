#include "evpp/inner_pre.h"

#include <string.h>

#include "evpp/fd_channel.h"
#include "evpp/libevent_headers.h"
#include "evpp/event_loop.h"

namespace evpp {
    static_assert(FdChannel::kReadable == EV_READ, "");
    static_assert(FdChannel::kWritable == EV_WRITE, "");

    FdChannel::FdChannel(EventLoop* l, int f, bool r, bool w)
        : loop_(l), attached_to_loop_(false), event_(NULL), fd_(f) {
        events_ = (r ? kReadable : 0) | (w ? kWritable : 0) | EV_PERSIST;
        event_ = new event;
        memset(event_, 0, sizeof(struct event));
    }

    void FdChannel::Close() {
        if (event_initialized(event_)) {
            event_del(event_);
            delete (event_);
            event_ = NULL;
        }
    }
    
    void FdChannel::AttachToLoop() {
        loop_->AssertInLoopThread();
        event_set(event_, fd_, events_, &FdChannel::HandleEvent, this);
        event_base_set(loop_->event_base(), event_);
        if (event_add(event_, NULL) != 0) {
            LOG_ERROR << "fd=" << fd_ << " with event " << EventsToString() << " attach to event loop failed";
        } else {
            attached_to_loop_ = true;
        }
    }

    void FdChannel::Update() {
        assert(attached_to_loop_);
        loop_->AssertInLoopThread();
        AttachToLoop();
    }

    std::string FdChannel::EventsToString() const {
        std::string s;
        if (events_ & kReadable) {
            s = "kReadable|";
        }

        if (events_ & kWritable) {
            s += "kWritable";
        }

        return s;
    }


    void FdChannel::HandleEvent(int fd, short which, void *v) {
        FdChannel *c = (FdChannel*)v;
        c->HandleEvent(fd, which);
    }

    void FdChannel::HandleEvent(int f, short which) {
        if ((which & kReadable) && read_fn_) {
            read_fn_(base::Timestamp::Now());
        }

        if ((which & kWritable) && write_fn_) {
            write_fn_();
        }
    }
}

