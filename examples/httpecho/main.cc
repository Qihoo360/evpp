#include <evpp/exp.h>
#include <evpp/http/http_server.h>

#ifdef _WIN32
#include "../echo/winmain-inl.h"
#endif

void RequestHandler(const evpp::http::ContextPtr& ctx, const evpp::http::HTTPSendResponseCallback& cb) {
    std::stringstream oss;
    oss << "func=" << __FUNCTION__ << " OK"
        << " ip=" << ctx->remote_ip << "\n"
        << " uri=" << ctx->uri << "\n"
        << " body=" << ctx->body.ToString() << "\n";
    cb(oss.str());
}

int main(int argc, char* argv[]) {
    std::vector<int> ports = {9009, 23456, 23457};
    int port = 29099;
    int thread_num = 0;

    if (argc > 1) {
        if (std::string("-h") == argv[1] || std::string("--h") == argv[1] || std::string("--help") == argv[1] || std::string("-help") == argv[1]) {
            std::cout << "usage : " << argv[0] << " <listen_port> <thread_num>\n";
            std::cout << " e.g. : " << argv[0] << " 80 24\n";
        }
    }

    if (argc == 2) {
        port = atoi(argv[1]);
    }
    ports.push_back(port);

    if (argc == 3) {
        port = atoi(argv[1]);
        thread_num = atoi(argv[2]);
    }
    evpp::http::HTTPServer server(thread_num);
    server.RegisterDefaultHandler(&RequestHandler);
    server.Start(ports);
    while (!server.IsStopped()) {
        usleep(1);
    }
    return 0;
}
