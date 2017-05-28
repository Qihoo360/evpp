#include "evpp/inner_pre.h"
#include "evpp/libevent.h"

#ifdef H_LIBEVENT_VERSION_14
struct event* event_new(struct event_base* base, evpp_socket_t fd, short events,
                        void(*cb)(int, short, void*), void* arg) {
    struct event* ev;
    ev = (struct event*)malloc(sizeof(struct event));

    if (ev == nullptr) {
        return nullptr;
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

#ifdef H_OS_MACOSX
void avoid_macosx_ranlib_complain() {
}
#endif
