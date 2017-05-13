
#include <evpp/http/http_server.h>
#include <evpp/httpc/request.h>
#include <evpp/httpc/response.h>

#include "../../../examples/winmain-inl.h"

void DefaultHandler(evpp::EventLoop* loop,
                    const evpp::http::ContextPtr& ctx,
                    const evpp::http::HTTPSendResponseCallback& cb) {
    std::stringstream oss;
    oss << "func=" << __FUNCTION__ << " OK"
        << " ip=" << ctx->remote_ip() << "\n"
        << " uri=" << ctx->uri() << "\n"
        << " body=" << ctx->body().ToString() << "\n";
    ctx->AddResponseHeader("Content-Type", "application/octet-stream");
    ctx->AddResponseHeader("Server", "evpp");
    cb(oss.str());
}

void ProxyHandler(evpp::EventLoop* loop,
                  const evpp::http::ContextPtr& ctx,
                  const evpp::http::HTTPSendResponseCallback& cb) {
    std::string uri = ctx->original_uri();
    size_t index = uri.find('?');
    if (index == std::string::npos) {
        cb("Format error. Please try again. The valid format is [http://127.0.0.1:9099/proxy?http://www.so.com/robots.txt]");
        return;
    }

    std::string proxy_url = uri.substr(index + 1);
    std::shared_ptr<evpp::httpc::GetRequest> r(new evpp::httpc::GetRequest(loop, proxy_url, evpp::Duration(3.0)));
    auto f = [cb, ctx, r](const std::shared_ptr<evpp::httpc::Response>& response) {
        ctx->set_response_http_code(response->http_code());
        std::string rc = response->body().ToString();
        cb(rc);
    };
    r->Execute(f);
}

int main(int argc, char* argv[]) {
    std::vector<int> ports = {9009, 23456, 23457};
    int port = 29099;
    int thread_num = 2;

    if (argc > 1) {
        if (std::string("-h") == argv[1] ||
                std::string("--h") == argv[1] ||
                std::string("-help") == argv[1] ||
                std::string("--help") == argv[1]) {
            std::cout << "usage : " << argv[0] << " <listen_port> <thread_num>\n";
            std::cout << " e.g. : " << argv[0] << " 8080 24\n";
            return 0;
        }
    }

    if (argc == 2) {
        port = atoi(argv[1]);
    } else if (argc == 3) {
        port = atoi(argv[1]);
        thread_num = atoi(argv[2]);
    }

    ports.push_back(port);

    evpp::http::Server server(thread_num);
    server.SetThreadDispatchPolicy(evpp::ThreadDispatchPolicy::kIPAddressHashing);
    server.RegisterDefaultHandler(&DefaultHandler);
    server.RegisterHandler("/proxy", &ProxyHandler);
    server.RegisterHandler("/echo",
                           [](evpp::EventLoop* loop,
                              const evpp::http::ContextPtr& ctx,
                              const evpp::http::HTTPSendResponseCallback& cb) {
        cb(ctx->body().ToString()); }
    );
    server.Init(ports);
    server.Start();
    while (!server.IsStopped()) {
        usleep(1);
    }
    return 0;
}
