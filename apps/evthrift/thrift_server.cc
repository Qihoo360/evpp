#include "thrift_server.h"

namespace evthrift {

ThriftServer::~ThriftServer() {}

void ThriftServer::serve() {
    server_.SetMessageCallback(std::bind(&ThriftServer::OnMessage,
                                         this, std::placeholders::_1,
                                         std::placeholders::_2));
    server_.Init();
    server_.Start();
}

void ThriftServer::stop() {
    server_.Stop();
}

void ThriftServer::OnConnection(const evpp::TCPConnPtr& conn) {
    if (conn->IsConnected()) {
        ThriftConnectionPtr ptr(new ThriftConn(this, conn));
        conn->set_context(evpp::Any(ptr));
    } else {
        conn->set_context(evpp::Any());
    }
}

void ThriftServer::OnMessage(const evpp::TCPConnPtr& conn, evpp::Buffer* buffer) {
    const evpp::Any& a = conn->context();
    if (a.IsEmpty()) {
        LOG_ERROR << "The evpp::TCPConn is not assoiated with a Thrift Connection";
        return;
    }

    ThriftConnectionPtr ptr = a.Get<ThriftConnectionPtr>();
    ptr->OnMessage(conn, buffer);
}

}