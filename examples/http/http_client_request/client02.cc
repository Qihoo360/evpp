#include <stdio.h>
#include <stdlib.h>

#include <evpp/event_loop_thread.h>

#include <evpp/httpc/request.h>
#include <evpp/httpc/conn.h>
#include <evpp/httpc/response.h>

#include "../../../examples/winmain-inl.h"

static int responsed = 0;
static void HandleHTTPResponse(const std::shared_ptr<evpp::httpc::Response>& response, evpp::httpc::GetRequest* request) {
    LOG_INFO << "http_code=" << response->http_code()
        << " URL=http://" << request->host() << request->uri()
        << " [" << response->body().ToString() << "]";
    const char* header = response->FindHeader("Connection");
    if (header) {
        LOG_INFO << "HTTP HEADER Connection=" << header;
    }
    responsed++;
    assert(request == response->request());
    delete request; // The request MUST BE deleted in EventLoop thread.
}

int main() {
    evpp::EventLoopThread t;
    t.Start(true);
    evpp::httpc::GetRequest* r = new evpp::httpc::GetRequest(t.loop(), "http://www.360.cn/robots.txt", evpp::Duration(5.0));
    r->Execute(std::bind(&HandleHTTPResponse, std::placeholders::_1, r));
    r = new evpp::httpc::GetRequest(t.loop(), "http://www.sohu.com/robots.txt", evpp::Duration(5.0));
    r->Execute(std::bind(&HandleHTTPResponse, std::placeholders::_1, r));
    r = new evpp::httpc::GetRequest(t.loop(), "http://www.so.com/status.html", evpp::Duration(5.0));
    r->Execute(std::bind(&HandleHTTPResponse, std::placeholders::_1, r));
    while (responsed != 3) {
        usleep(1);
    }

    t.Stop(true);
    LOG_INFO << "EventLoopThread stopped.";
    return 0;
}
