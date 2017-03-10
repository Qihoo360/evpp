#include <event2/listener.h>
#include <event2/bufferevent.h>
#include <event2/buffer.h>

#include <arpa/inet.h>
#include <netinet/tcp.h>

#include <signal.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

static void set_tcp_no_delay(evutil_socket_t fd)
{
  int one = 1;
  setsockopt(fd, IPPROTO_TCP, TCP_NODELAY,
      &one, sizeof one);
}

static void signal_cb(evutil_socket_t fd, short what, void *arg)
{
  struct event_base *base = arg;
  printf("stop\n");

  event_base_loopexit(base, NULL);
}

static void echo_read_cb(struct bufferevent *bev, void *ctx)
{
  /* This callback is invoked when there is data to read on bev. */
  struct evbuffer *input = bufferevent_get_input(bev);
  struct evbuffer *output = bufferevent_get_output(bev);

  /* Copy all the data from the input buffer to the output buffer. */
  evbuffer_add_buffer(output, input);
}

static void echo_event_cb(struct bufferevent *bev, short events, void *ctx)
{
  struct evbuffer *output = bufferevent_get_output(bev);
  size_t remain = evbuffer_get_length(output);
  if (events & BEV_EVENT_ERROR) {
    perror("Error from bufferevent");
  }
  if (events & (BEV_EVENT_EOF | BEV_EVENT_ERROR)) {
    printf("closing, remain %zd\n", remain);
    bufferevent_free(bev);
  }
}

static void accept_conn_cb(struct evconnlistener *listener,
    evutil_socket_t fd, struct sockaddr *address, int socklen,
    void *ctx)
{
  /* We got a new connection! Set up a bufferevent for it. */
  struct event_base *base = evconnlistener_get_base(listener);
  struct bufferevent *bev = bufferevent_socket_new(
      base, fd, BEV_OPT_CLOSE_ON_FREE);
  set_tcp_no_delay(fd);

  bufferevent_setcb(bev, echo_read_cb, NULL, echo_event_cb, NULL);

  bufferevent_enable(bev, EV_READ|EV_WRITE);
}

int main(int argc, char **argv)
{
  struct event_base *base;
  struct evconnlistener *listener;
  struct sockaddr_in sin;
  struct event *evstop;

  int port = 9876;

  if (argc > 1) {
    port = atoi(argv[1]);
  }
  if (port<=0 || port>65535) {
    puts("Invalid port");
    return 1;
  }

  signal(SIGPIPE, SIG_IGN);

  base = event_base_new();
  if (!base) {
    puts("Couldn't open event base");
    return 1;
  }

  evstop = evsignal_new(base, SIGHUP, signal_cb, base);
  evsignal_add(evstop, NULL);

  /* Clear the sockaddr before using it, in case there are extra
   *          * platform-specific fields that can mess us up. */
  memset(&sin, 0, sizeof(sin));
  /* This is an INET address */
  sin.sin_family = AF_INET;
  /* Listen on 0.0.0.0 */
  sin.sin_addr.s_addr = htonl(0);
  /* Listen on the given port. */
  sin.sin_port = htons(port);

  listener = evconnlistener_new_bind(base, accept_conn_cb, NULL,
      LEV_OPT_CLOSE_ON_FREE|LEV_OPT_REUSEABLE, -1,
      (struct sockaddr*)&sin, sizeof(sin));
  if (!listener) {
    perror("Couldn't create listener");
    return 1;
  }

  event_base_dispatch(base);

  evconnlistener_free(listener);
  event_free(evstop);
  event_base_free(base);
  return 0;
}
