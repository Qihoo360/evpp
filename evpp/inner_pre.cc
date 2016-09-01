#include "evpp/inner_pre.h"

#include "evpp/libevent_headers.h"

#ifdef H_WINDOWS_API
#pragma comment(lib, "event.lib")
#if EVENT__NUMERIC_VERSION >= 0x02010500
#pragma comment(lib, "event_core.lib")
#pragma comment(lib, "event_extra.lib")
#endif
#pragma comment(lib,"Ws2_32.lib")
#pragma comment(lib,"libglog_static.lib")
#endif


#ifndef H_OS_WINDOWS

//TODO ignore SIGPIPE add sig_pipe
#include <signal.h>
// set signal handler
void sig_pipe(int id) {
    // SIGPIPE
    //H_LOG_NAME_DEBUG( "", "a pipe arrived.");
    // do nothing.
    //printf( "signal pipe:%d", id );
}
void sig_child(int) {
    //H_LOG_NAME_DEBUG( "", "a child thread terminated." );
    // do nothing.
}

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
