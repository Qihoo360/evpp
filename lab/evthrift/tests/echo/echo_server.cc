
#include "evthrift/thrift_server.h"

#include "gen-cpp/Echo.h"

using namespace echo;

class EchoHandler : virtual public EchoIf {
public:
    EchoHandler() {}

    void echo(std::string& str, const std::string& s) {
        LOG_INFO << "EchoHandler::echo:" << s;
        str = s;
    }


    void ping() {
        LOG_INFO << "EchoHandler::ping ...";
    }

};

int main(int argc, char **argv) {
    evpp::EventLoop loop;
    std::string name("EchoServer");
    std::string addr = "0.0.0.0:9099";

    boost::shared_ptr<EchoHandler> handler(new EchoHandler());
    boost::shared_ptr<evthrift::TProcessor> processor(new EchoProcessor(handler));

    evthrift::ThriftServer server(processor, &loop, addr, name, 4);
    server.start();
    loop.Run();
    return 0;
}


#include "../../../../examples/winmain-inl.h"
