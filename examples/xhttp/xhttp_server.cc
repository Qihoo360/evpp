
#include <iostream>
#include <string>
#include <evpp/stringutil.h>
#include <evpp/event_loop.h>
#include <evpp/xhttp/xhttp.h>
#include <evpp/xhttp/httpserver.h>
#include <evpp/xhttp/httpconnection.h>

int main(int argc, char const *argv[])
{
    std::string s = "This is test";
    std::cout << s << std::endl;
    std::cout << evpp::StringUtil::upper(s) << std::endl;

    evpp::EventLoop* loop = new evpp::EventLoop();
    evpp::xhttp::HttpServer httpServer(loop, "0.0.0.0:9999", "test", 4);
    httpServer.Init();
    httpServer.Start();

    loop->Run();

    delete loop;

    return 0;
}
