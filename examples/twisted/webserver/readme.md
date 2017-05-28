Here is a C++ HTTP web server which implements the example of [twisted](http://twistedmatrix.com/trac/#webserver)

```C++
#include <atomic>

#include <evpp/http/http_server.h>

std::atomic<int> request_count;

void DefaultHandler(evpp::EventLoop* loop,
                    const evpp::http::ContextPtr& ctx,
                    const evpp::http::HTTPSendResponseCallback& cb) {
    ctx->AddResponseHeader("Content-Type", "text/plain");
    std::ostringstream os;
    os << "I am request #" << ++request_count << "\n";
    cb(os.str());
}

int main(int argc, char* argv[]) {
    int port = 8080;
    if (argc == 2) {
        port = atoi(argv[1]);
    }

    evpp::http::Server server;
    server.RegisterDefaultHandler(&DefaultHandler);
    server.Init(port);
    server.Start();
    while (!server.IsStopped()) {
        usleep(1);
    }
    return 0;
}

```
