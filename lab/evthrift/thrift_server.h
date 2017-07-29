#pragma once

#include <map>

#include <thrift/server/TServer.h>

#include <evpp/tcp_server.h>

#include "thrift_conn.h"

namespace evthrift {

using apache::thrift::TProcessor;
using apache::thrift::TProcessorFactory;
using apache::thrift::protocol::TProtocolFactory;
using apache::thrift::server::TServer;
using apache::thrift::server::TServerTransport;
using apache::thrift::transport::TTransportFactory;

class ThriftServer : public TServer {
public:
    template <typename Processor>
    ThriftServer(const boost::shared_ptr<Processor>& processor, // or TProcessorFactory
                    evpp::EventLoop* eventloop,
                    const std::string& listen_addr/*ip:port*/,
                    const std::string& name,
                    uint32_t thread_num)
        : TServer(processor),
        server_(eventloop, listen_addr, name, thread_num) {
        server_.SetConnectionCallback(std::bind(&ThriftServer::OnConnection,
                                                this, std::placeholders::_1));
    }

    template <typename Processor>
    ThriftServer(const boost::shared_ptr<Processor>& processor, // or TProcessorFactory
                    const boost::shared_ptr<TServerTransport>& transport,
                    evpp::EventLoop* eventloop,
                    const std::string& listen_addr/*ip:port*/,
                    const std::string& name,
                    uint32_t thread_num)
        : TServer(processor, transport),
        server_(eventloop, listen_addr, name, thread_num) {
        server_.SetConnectionCallback(std::bind(&ThriftServer::OnConnection,
                                                this, std::placeholders::_1));
    }

    template <typename Processor>
    ThriftServer(const boost::shared_ptr<Processor>& processor,
                 const boost::shared_ptr<TServerTransport>& serverTransport,
                 const boost::shared_ptr<TTransportFactory>& transportFactory,
                 const boost::shared_ptr<TProtocolFactory>& protocolFactory,
                    evpp::EventLoop* eventloop,
                    const std::string& listen_addr/*ip:port*/,
                    const std::string& name,
                    uint32_t thread_num)
        : TServer(processor, serverTransport, transportFactory, protocolFactory),
        server_(eventloop, listen_addr, name, thread_num) {
        server_.SetConnectionCallback(std::bind(&ThriftServer::OnConnection,
                                                this, std::placeholders::_1));
    }

    template <typename Processor>
    ThriftServer(const boost::shared_ptr<Processor>& processor,
                 const boost::shared_ptr<TServerTransport>& serverTransport,
                 const boost::shared_ptr<TTransportFactory>& inputTransportFactory,
                 const boost::shared_ptr<TTransportFactory>& outputTransportFactory,
                 const boost::shared_ptr<TProtocolFactory>& inputProtocolFactory,
                 const boost::shared_ptr<TProtocolFactory>& outputProtocolFactory,
                    evpp::EventLoop* eventloop,
                    const std::string& listen_addr/*ip:port*/,
                    const std::string& name,
                    uint32_t thread_num)
        : TServer(processor, serverTransport, inputTransportFactory, outputTransportFactory, inputProtocolFactory, outputProtocolFactory),
        server_(eventloop, listen_addr, name, thread_num) {
        server_.SetConnectionCallback(std::bind(&ThriftServer::OnConnection,
                                                this, std::placeholders::_1));
    }

    virtual ~ThriftServer();

    void serve();
    void start() {
        serve();
    }

    void stop();

private:
    friend class ThriftConn;

    void OnConnection(const evpp::TCPConnPtr& conn);
    void OnMessage(const evpp::TCPConnPtr& conn,
                   evpp::Buffer* buffer);
private:
    evpp::TCPServer server_;
};

}
