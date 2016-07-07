#include "evpp/inner_pre.h"

#include <string.h>

#include "evpp/fd_channel.h"
#include "evpp/libevent_headers.h"
#include "evpp/event_loop.h"

namespace evpp {
    static_assert(FdChannel::kReadable == EV_READ, "");
    static_assert(FdChannel::kWritable == EV_WRITE, "");

    FdChannel::FdChannel(EventLoop* l, int f, bool r, bool w)
        : loop_(l), attached_(false), event_(NULL), fd_(f) {
        events_ = (r ? kReadable : 0) | (w ? kWritable : 0);
        event_ = new event;
        memset(event_, 0, sizeof(struct event));
    }

    FdChannel::~FdChannel() {
        LOG_INFO << "FdChannel::~FdChannel() this=" << this << " fd=" << fd_;
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
        assert(!IsNoneEvent());
        assert(loop_->IsInLoopThread());
        if (attached_) {
            // FdChannel::Update ���ܻᱻ��ε��ã�����������Ա��� event_add ����ε���
            DetachFromLoop();
        }

        ::event_set(event_, fd_, events_ | EV_PERSIST, &FdChannel::HandleEvent, this);
        ::event_base_set(loop_->event_base(), event_);
        if (::event_add(event_, NULL) == 0) {
            LOG_TRACE << "this=" << this << " fd=" << fd_ << " watching event " << EventsToString();
            attached_ = true;
        } else {
            LOG_ERROR << "this=" << this << " fd=" << fd_ << " with event " << EventsToString() << " attach to event loop failed";
        }
    }

    void FdChannel::EnableReadEvent() {
        int events = events_;
        events_ |= kReadable;
        if (events_ != events) {
            Update();
        }
    }

    void FdChannel::EnableWriteEvent() {
        int events = events_;
        events_ |= kWritable;
        if (events_ != events) {
            Update();
        }
    }

    void FdChannel::DisableReadEvent() {
        int events = events_;
        events_ &= (~kReadable);
        if (events_ != events) {
            Update();
        }
    }

    void FdChannel::DisableWriteEvent() {
        int events = events_;
        events_ &= (~kWritable);
        if (events_ != events) {
            Update();
        }
    }

    void FdChannel::DisableAllEvent() {
        if (events_ == kNone) {
            return;
        }
        events_ = kNone;
        Update();
    }

    void FdChannel::DetachFromLoop() {
        assert(loop_->IsInLoopThread());
        assert(attached_);
        if (::event_del(event_) == 0) {
            attached_ = false;
            LOG_TRACE << "DetachFromLoop this=" << this << " fd=" << fd_ << " detach from event loop";
        } else {
            LOG_ERROR << "DetachFromLoop this=" << this << "fd=" << fd_ << " with event " << EventsToString() << " detach from event loop failed";
        }
    }

    void FdChannel::Update() {
        assert(loop_->IsInLoopThread());
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
