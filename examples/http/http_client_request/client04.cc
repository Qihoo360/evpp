#include <stdio.h>
#include <stdlib.h>

#include <evpp/event_loop_thread.h>

#include <evpp/httpc/conn_pool.h>
#include <evpp/httpc/request.h>
#include <evpp/httpc/conn.h>
#include <evpp/httpc/response.h>

#include "../../../examples/winmain-inl.h"

static int responsed = 0;
static int retried = 0;
static void HandleHTTPResponse(const std::shared_ptr<evpp::httpc::Response>& r, evpp::httpc::Request* req) {
    LOG_INFO << "http_code=" << r->http_code() << " [" << r->body().ToString() << "]";
    responsed++;
    if (retried < 3) {
        retried++;
        req->Execute(std::bind(&HandleHTTPResponse, std::placeholders::_1, req));
    } else {
        delete req;
    }
}

int main() {
    evpp::EventLoopThread t;
    t.Start(true);
    evpp::httpc::Request* r = new evpp::httpc::Request(t.loop(), "http://www.360.cn/robots.txt", "", evpp::Duration(2.0));
    LOG_INFO << "Do http request";
    r->Execute(std::bind(&HandleHTTPResponse, std::placeholders::_1, r));

    while (responsed != 4) {
        usleep(1);
    }

    t.Stop(true);
    LOG_INFO << "EventLoopThread stopped.";
    return 0;
}
