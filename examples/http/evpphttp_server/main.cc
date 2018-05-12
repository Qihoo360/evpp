#include <evpp/evpphttp/service.h>
#include <iostream>

static int g_port = 29099;
void DefaultHandler(evpp::EventLoop* loop,
                    evpp::evpphttp::HttpRequest& ctx,
                    const evpp::evpphttp::HTTPSendResponseCallback& cb) {
    std::stringstream oss;
    oss << "func=" << __FUNCTION__ << " OK"
        << " ip=" << ctx.remote_ip << "\n"
        << " uri=" << ctx.url_path() << "\n"
        << " body=" << ctx.body.ToString() << "\n";
	std::map<std::string, std::string> feild_value = {
		{"Content-Type", "application/octet-stream"},
        {"Server", "evpp"}
	};
    cb(200, feild_value, oss.str());
}


int main(int argc, char* argv[]) {
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
        g_port = atoi(argv[1]);
    } else if (argc == 3) {
        g_port = atoi(argv[1]);
        thread_num = atoi(argv[2]);
    } 
    evpp::evpphttp::Service server(std::string("0.0.0.0:") + std::to_string(g_port), "test", thread_num);
    server.RegisterHandler("/echo", &DefaultHandler);
    if (!server.Start()) {
		std::cout << "start serv failed\n";
		return -1;
	}
	std::cout << "start serv on 0.0.0.0:" << g_port << " suc\n";
    while (!server.IsStopped()) {
        usleep(1);
    }
    return 0;
}
