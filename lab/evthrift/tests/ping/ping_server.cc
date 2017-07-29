#include <thrift/protocol/TCompactProtocol.h>

#include "evthrift/thrift_server.h"

#include "gen-cpp/Ping.h"

using apache::thrift::protocol::TCompactProtocolFactory;

using namespace ping;

class PingHandler : virtual public PingIf {
public:
    PingHandler() {}

    void ping() {
        LOG_INFO << "ping";
    }

};

int main(int argc, char **argv) {
    evpp::EventLoop loop;
    std::string name("EchoServer");
    std::string addr = "0.0.0.0:9099";

    boost::shared_ptr<PingHandler> handler(new PingHandler());
    boost::shared_ptr<evthrift::TProcessor> processor(new PingProcessor(handler));

    evthrift::ThriftServer server(processor, &loop, addr, name, 4);
    server.start();
    loop.Run();
    return 0;

    return 0;
}


#include "../../../../examples/winmain-inl.h"
