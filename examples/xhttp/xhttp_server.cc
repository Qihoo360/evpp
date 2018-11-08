
#include <iostream>
#include <string>
#include <evpp/stringutil.h>
#include <evpp/event_loop.h>
#include <evpp/xhttp/xhttp.h>
#include <evpp/xhttp/httpresponse.h>
#include <evpp/xhttp/httpserver.h>
#include <evpp/xhttp/httpconnection.h>


using namespace evpp;

using namespace evpp::xhttp;

int main(int argc, char const *argv[])
{
    std::string s = "This is test";
    std::cout << s << std::endl;
    std::cout << evpp::StringUtil::upper(s) << std::endl;

    evpp::EventLoop* loop = new evpp::EventLoop();
    evpp::xhttp::HttpServer httpServer(loop, "0.0.0.0:9999", "test", 4);
    httpServer.setHttpCallback("/test", [](const HttpConnectionPtr_t& conn, const HttpRequest& req) {
        std::cout << "OnHttpRequest" << std::endl;
        HttpResponse resp(200);
        resp.body = "Hello World!";

        conn->send(resp);
    });
    httpServer.Init();
    httpServer.Start();

    loop->Run();

    delete loop;

    return 0;
}
