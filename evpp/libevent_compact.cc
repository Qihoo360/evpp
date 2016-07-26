#include "evpp/inner_pre.h"
#include "evpp/libevent_headers.h"

#include <map>
#include <thread>
#include <mutex>

#ifdef H_LIBEVENT_VERSION_14
struct event* event_new(struct event_base* base, int fd, short events,
                        void(*cb)(int, short, void*), void* arg) {
    struct event* ev;
    ev = (struct event*)malloc(sizeof(struct event));

    if (ev == NULL) {
        return NULL;
    }

    ::event_set(ev, fd, events, cb, arg);
    ::event_base_set(base, ev);
    return ev;
}

void event_free(struct event* ev) {
    evpp::EventDel(ev);
    free(ev);
}


struct evhttp_connection* evhttp_connection_base_new(struct event_base* base, struct evdns_base* dnsbase, const char* address, unsigned short port) {
    struct evhttp_connection* evhttp_conn = evhttp_connection_new(address, port);
    evhttp_connection_set_base(evhttp_conn, base);
    return evhttp_conn;
}

#endif

namespace evpp {

#ifdef H_DEBUG_MODE
static std::map<struct event*, std::thread::id> evmap;
static std::mutex mutex;
#else
    static int count;
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
#endif
    return 0;
}

}