
#include "evpp/exp.h"
#include "test_common.h"

#include "evpp/libevent_headers.h"
#include "evpp/libevent_watcher.h"
#include "evpp/event_loop.h"
#include "evpp/tcp_server.h"

#include <thread>

TEST_UNIT(testATCPServer) {
    evpp::EventLoop loop;
    evpp::TCPServer tsrv(&loop, "127.0.0.1:9099", "tcp_server", 1);
    tsrv.Start();
    loop.Run();
}

