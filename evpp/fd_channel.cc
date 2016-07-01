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

    FdChannel::~FdChannel() {
        LOG_INFO << "FdChannel::~FdChannel() fd=" << fd_;
        assert(event_ == NULL);
    }

    void FdChannel::Close() {
        if (event_) {
            if (event_initialized(event_)) {
                ::event_del(event_);
            }
            delete (event_);
            event_ = NULL;
        }
    }

    void FdChannel::AttachToLoop() {
        loop_->AssertInLoopThread();
        if (attached_to_loop_) {
            // FdChannel::Update 可能会被多次调用，这样处理可以避免 event_add 被多次调用
            ::event_del(event_);
            attached_to_loop_ = false;
        }

        ::event_set(event_, fd_, events_, &FdChannel::HandleEvent, this);
        ::event_base_set(loop_->event_base(), event_);
        if (::event_add(event_, NULL) == 0) {
            LOG_TRACE << "fd=" << fd_ << " watching event " << EventsToString();
            attached_to_loop_ = true;
        } else {
            LOG_ERROR << "fd=" << fd_ << " with event " << EventsToString() << " attach to event loop failed";
        }
    }

    void FdChannel::DetachFromLoop() {
        loop_->AssertInLoopThread();
        if (::event_del(event_) == 0) {
            attached_to_loop_ = false;
        } else {
            LOG_ERROR << "fd=" << fd_ << " with event " << EventsToString() << " detach to event loop failed";
        }
    }

    void FdChannel::Update() {
        assert(attached_to_loop_);
        loop_->AssertInLoopThread();
        if (IsNoneEvent()) {
            DetachFromLoop();
        } else {
            AttachToLoop();
        }
    }

    std::string FdChannel::EventsToString() const {
        std::string s;
        if (events_ & kReadable) {
            s = "kReadable";
        }

        if (events_ & kWritable) {
            if (!s.empty()) {
                s += "|";
            }
            s += "kWritable";
        }

        return s;
    }

    void FdChannel::HandleEvent(int sockfd, short which, void *v) {
        FdChannel *c = (FdChannel*)v;
        c->HandleEvent(sockfd, which);
    }

    void FdChannel::HandleEvent(int sockfd, short which) {
        assert(sockfd == fd_);
        LOG_TRACE << "HandleEvent fd=" << sockfd << " " << EventsToString();
        if ((which & kReadable) && read_fn_) {
            read_fn_(Timestamp::Now());
        }

        if ((which & kWritable) && write_fn_) {
            write_fn_();
        }
    }
}
