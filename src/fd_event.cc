#include "evpp/inner_pre.h"

#include "evpp/fd_event.h"
#include "evpp/libevent_headers.h"

namespace evpp {
    static_assert(FdEvent::kReadable == EV_READ, "");
    static_assert(FdEvent::kWritable == EV_WRITE, "");

    FdEvent::FdEvent(struct event_base* loop)
        : loop_(loop)
        , ev_(NULL)
        , flags_(0)
        , active_(false) {}

    FdEvent::~FdEvent() {
        LOG_INFO << "FdEvent::~Impl called, call Cancel";
        Cancel();
    }

    void FdEvent::Start(int fd, int events, uint32_t timeout_us) {

        LOG_INFO << "FdEvent Start called, fd: " << fd;

        if ((events & (kReadable | kWritable)) == 0) {
            LOG_WARN << "invalid events, events: " << events;
            return;
        }

        Cancel();
        assert(active_ == false);

        if (fd < 0) {
            return;
        }
        short e = 0;
        if (events & kReadable) {
            LOG_INFO << "start with read event, fd: " << fd;
            e |= EV_READ;
        }

        if (events & kWritable) {
            LOG_INFO << "start with write event, fd: " << fd;
            e |= EV_WRITE;
        }

        ev_ = event_new(loop_
            , fd
            , e /*no EV_PERSIST, because FdEvent is a once event*/
            , Notify
            , (void *)this);
        CHECK_NOTNULL(ev_);

        LOG_INFO << "event_new called, ev_ : " << ev_;


        struct timeval tv;
        struct timeval* ptv = NULL;
        if (timeout_us > 0) {
            tv = evpp::timevalconv(timeout_us);
            ptv = &tv;
        }

        int rc = event_add(ev_, ptv);
        if (rc != 0) {
            LOG_ERROR << "event_add error";
            return;
        }
        active_ = true;
    }

    void FdEvent::Cancel() {

        LOG_INFO << "FdEvent Cancel called";

        if (!active_) {
            return;
        }

        active_ = false;

        if (ev_ != NULL) {
            LOG_INFO << "call event_del and event_free, ev_: " << ev_;
            int rc = event_del(ev_);
            if (rc != 0) {
                LOG_ERROR << "event_add error";
                return;
            }
            event_free(ev_);
            ev_ = NULL;
        } else {
            LOG_INFO << "ev_ is NULL, nothing to do";
        }

        handler_ = Functor();
    }

    void FdEvent::Notify(int fd, short what, void *arg) {
        CHECK_NOTNULL(arg);

        FdEvent *self = static_cast<FdEvent *>(arg);

        if (what & EV_READ) {
            LOG_INFO << "read event occurred, fd: " << fd;
            self->flags_ |= kReadable;
        } else if (what & EV_WRITE) {
            LOG_INFO << "write event occurred, fd: " << fd;
            self->flags_ |= kWritable;
        } else if (what & EV_TIMEOUT) {
            LOG_INFO << "timeout event occurred, fd: " << fd;
        } else {
            LOG_ERROR << "invalid event type:" << what
                << ", fd: " << fd;
            return;
        }


        LOG_INFO << "active: "
            << self->active_
            << ", fd: " << fd;

        if (self->active_) {
            LOG_INFO << "Notify call handler, fd: " << fd;
            self->handler_(self->flags_);
        } else {
            LOG_WARN << "FDEvent, Notify nothing to do, fd: " << fd;
        }
    }

    void FdEvent::AsyncWait(int fd, int events, const Functor& handler, uint32_t timeout_us) {
        LOG_INFO << "FdEvent AsyncWait with no time, fd: " << fd;

        if (active_) {
            Cancel();
        }

        handler_ = handler;

        Start(fd, events, timeout_us);
    }
}
