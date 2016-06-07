#include "evpp/inner_pre.h"
#include "evpp/libevent_headers.h"

#ifdef _LIBEVENT14
struct event * event_new(struct event_base *base, int fd, short events,
    void(*cb)(int, short, void *), void *arg) {
    struct event *ev;
    ev = (struct event*)malloc(sizeof(struct event));
    if (ev == NULL)
        return (NULL);
    event_base_set(base, ev);
    event_set(ev, fd, events, cb, arg);
    return (ev);
}

void event_free(struct event *ev) {
    event_del(ev);
    free(ev);
}

#endif

