#include "evpp/inner_pre.h"
#include "evpp/libevent_headers.h"

#ifdef H_LIBEVENT_VERSION_14
struct event* event_new(struct event_base *base, int fd, short events,
    void(*cb)(int, short, void *), void *arg) {
    struct event *ev;
    ev = (struct event*)malloc(sizeof(struct event));
    if (ev == NULL)
        return NULL;
    ::event_set(ev, fd, events, cb, arg);
    ::event_base_set(base, ev);
    return ev;
}

void event_free(struct event *ev) {
    ::event_del(ev);
    free(ev);
}

#endif

