#include "evpp/inner_pre.h"

#include "evpp/libevent.h"

#ifdef H_OS_WINDOWS
#pragma comment(lib,"Ws2_32.lib")
#endif

#ifndef H_OS_WINDOWS
#include <signal.h>
#endif

#include <map>
#include <thread>
#include <mutex>

namespace evpp {

namespace {
struct OnStartup {
    OnStartup() {
#ifndef H_OS_WINDOWS
        if (signal(SIGPIPE, SIG_IGN) == SIG_ERR) {
            LOG_ERROR << "SIGPIPE set failed.";
            exit(-1);
        }
        LOG_INFO << "ignore SIGPIPE";
#endif
    }
    ~OnStartup() {
    }
} __s_onstartup;
}


#ifdef H_DEBUG_MODE
static std::map<struct event*, std::thread::id> evmap;
static std::mutex mutex;
#endif

int EventAdd(struct event* ev, const struct timeval* timeout) {
#ifdef H_DEBUG_MODE
    {
        std::lock_guard<std::mutex> guard(mutex);
        if (evmap.find(ev) == evmap.end()) {
            auto id = std::this_thread::get_id();
            evmap[ev] = id;
        } else {
            LOG_ERROR << "Event " << ev << " fd=" << ev->ev_fd << " event_add twice!";
            assert("event_add twice");
        }
    }
    LOG_DEBUG << "event_add ev=" << ev << " fd=" << ev->ev_fd << " user_ptr=" << ev->ev_arg << " tid=" << std::this_thread::get_id();
#endif
    return event_add(ev, timeout);
}

int EventDel(struct event* ev) {
#ifdef H_DEBUG_MODE
    {
        std::lock_guard<std::mutex> guard(mutex);
        auto it = evmap.find(ev);
        if (it == evmap.end()) {
            LOG_ERROR << "Event " << ev << " fd=" << ev->ev_fd << " not exist in event loop, maybe event_del twice.";
            assert("event_del twice");
        } else {
            auto id = std::this_thread::get_id();
            if (id != it->second) {
                LOG_ERROR << "Event " << ev << " fd=" << ev->ev_fd << " deleted in different thread.";
                assert(it->second == id);
            }
            evmap.erase(it);
        }
    }
    LOG_DEBUG << "event_del ev=" << ev << " fd=" << ev->ev_fd << " user_ptr=" << ev->ev_arg << " tid=" << std::this_thread::get_id();
#endif
    return event_del(ev);
}

int GetActiveEventCount() {
#ifdef H_DEBUG_MODE
    return evmap.size();
#else
    return 0;
#endif
}

}
