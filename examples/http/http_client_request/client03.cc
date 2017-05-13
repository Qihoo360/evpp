#include <stdio.h>
#include <stdlib.h>

#include <evpp/event_loop_thread.h>

#include <evpp/httpc/conn_pool.h>
#include <evpp/httpc/request.h>
#include <evpp/httpc/conn.h>
#include <evpp/httpc/response.h>

#include "../../../examples/winmain-inl.h"

static bool responsed = false;
static void HandleHTTPResponse(const std::shared_ptr<evpp::httpc::Response>& response, evpp::httpc::Request* request) {
    LOG_INFO << "http_code=" << response->http_code() << " [" << response->body().ToString() << "]";
    std::string header = response->FindHeader("Connection");
    LOG_INFO << "HTTP HEADER Connection=" << header;
    responsed = true;
    assert(request == response->request());
    delete request; // The request MUST BE deleted in EventLoop thread.
}

int main() {
    evpp::EventLoopThread t;
    t.Start(true);
    std::shared_ptr<evpp::httpc::ConnPool> pool(new evpp::httpc::ConnPool("www.360.cn", 80, evpp::Duration(2.0)));
    evpp::httpc::Request* r = new evpp::httpc::Request(pool.get(), t.loop(), "/robots.txt", "");
    LOG_INFO << "Do http request";
    r->Execute(std::bind(&HandleHTTPResponse, std::placeholders::_1, r));

    while (!responsed) {
        usleep(1);
    }

    pool->Clear();
    pool.reset();
    t.Stop(true);
    LOG_INFO << "EventLoopThread stopped.";
    return 0;
}
