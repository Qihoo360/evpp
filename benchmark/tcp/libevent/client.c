#include <event2/event.h>
#include <event2/bufferevent.h>
#include <event2/buffer.h>

#include <sys/socket.h>
#include <netinet/tcp.h>

#include <string.h>
#include <stdlib.h>

int64_t total_bytes_read = 0;
int64_t total_messages_read = 0;

static void set_tcp_no_delay(evutil_socket_t fd)
{
  int one = 1;
  setsockopt(fd, IPPROTO_TCP, TCP_NODELAY,
      &one, sizeof one);
}

static void timeoutcb(evutil_socket_t fd, short what, void *arg)
{
  struct event_base *base = arg;
  printf("timeout\n");
  
  event_base_loopexit(base, NULL);
}

static void readcb(struct bufferevent *bev, void *ctx)
{
  /* This callback is invoked when there is data to read on bev. */
  struct evbuffer *input = bufferevent_get_input(bev);
  struct evbuffer *output = bufferevent_get_output(bev);

  ++total_messages_read;
  total_bytes_read += evbuffer_get_length(input);

  /* Copy all the data from the input buffer to the output buffer. */
  evbuffer_add_buffer(output, input);
}

static void eventcb(struct bufferevent *bev, short events, void *ptr)
{
  if (events & BEV_EVENT_CONNECTED) {
    evutil_socket_t fd = bufferevent_getfd(bev);
    set_tcp_no_delay(fd);
  } else if (events & BEV_EVENT_ERROR) {
    printf("NOT Connected\n");
  }
}

int main(int argc, char **argv)
{
  struct event_base *base;
  struct bufferevent **bevs;
  struct sockaddr_in sin;
  struct event *evtimeout;
  struct timeval timeout;
  int i;

  if (argc != 5) {
    fprintf(stderr, "Usage: client <port> <blocksize> ");
    fprintf(stderr, "<sessions> <time>\n");
    return 1;
  }

  int port = atoi(argv[1]);
  int block_size = atoi(argv[2]);
  int session_count = atoi(argv[3]);
  int seconds = atoi(argv[4]);
  timeout.tv_sec = seconds;
  timeout.tv_usec = 0;

  base = event_base_new();
  if (!base) {
    puts("Couldn't open event base");
    return 1;
  }

  char* message = malloc(block_size);
  for (i = 0; i < block_size; ++i) {
    message[i] = i % 128;
  }

  evtimeout = evtimer_new(base, timeoutcb, base);
  evtimer_add(evtimeout, &timeout);

  memset(&sin, 0, sizeof(sin));
  sin.sin_family = AF_INET;
  sin.sin_addr.s_addr = htonl(0x7f000001); /* 127.0.0.1 */
  sin.sin_port = htons(port);

  bevs = malloc(session_count * sizeof(struct bufferevent *));
  for (i = 0; i < session_count; ++i) {
    struct bufferevent *bev = bufferevent_socket_new(base, -1, BEV_OPT_CLOSE_ON_FREE);

    bufferevent_setcb(bev, readcb, NULL, eventcb, NULL);
    bufferevent_enable(bev, EV_READ|EV_WRITE);
    evbuffer_add(bufferevent_get_output(bev), message, block_size);

    if (bufferevent_socket_connect(bev,
          (struct sockaddr *)&sin, sizeof(sin)) < 0) {
      /* Error starting connection */
      bufferevent_free(bev);
      puts("error connect");
      return -1;
    }
    bevs[i] = bev;
  }

  event_base_dispatch(base);

  for (i = 0; i < session_count; ++i) {
    bufferevent_free(bevs[i]);
  }
  free(bevs);
  event_free(evtimeout);
  event_base_free(base);
  free(message);

  printf("%zd total bytes read\n", total_bytes_read);
  printf("%zd total messages read\n", total_messages_read);
  printf("%.3f average messages size\n",
      (double)total_bytes_read / total_messages_read);
  printf("%.3f MiB/s throughtput\n",
      (double)total_bytes_read / (timeout.tv_sec * 1024 * 1024));
  return 0;
}

