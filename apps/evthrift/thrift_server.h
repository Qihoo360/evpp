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
        server_(eventloop, addr, name, thread_num),
        numWorkerThreads_(thread_num),
        workerThreadPool_(eventloop, thread_num) {
        server_.SetConnectionCallback(std::bind(&ThriftServer::onConnection,
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
        server_(eventloop, addr, name, thread_num),
        numWorkerThreads_(thread_num),
        workerThreadPool_(eventloop, thread_num) {
        server_.SetConnectionCallback(std::bind(&ThriftServer::onConnection,
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
        server_(eventloop, addr, name, thread_num),
        numWorkerThreads_(thread_num),
        workerThreadPool_(eventloop, thread_num) {
        server_.SetConnectionCallback(std::bind(&ThriftServer::onConnection,
                                                this, std::placeholders::_1));
    }

    template <typename Processor>
    ThriftServer(const boost::shared_ptr<Processor>& Processor,
                 const boost::shared_ptr<TServerTransport>& serverTransport,
                 const boost::shared_ptr<TTransportFactory>& inputTransportFactory,
                 const boost::shared_ptr<TTransportFactory>& outputTransportFactory,
                 const boost::shared_ptr<TProtocolFactory>& inputProtocolFactory,
                 const boost::shared_ptr<TProtocolFactory>& outputProtocolFactory,
                    evpp::EventLoop* eventloop,
                    const std::string& listen_addr/*ip:port*/,
                    const std::string& name,
                    uint32_t thread_num)
        : TServer(processor, serverTransport, transportFactory, protocolFactory, inputProtocolFactory, outputProtocolFactory),
        server_(eventloop, addr, name, thread_num),
        numWorkerThreads_(thread_num),
        workerThreadPool_(eventloop, thread_num) {
        server_.SetConnectionCallback(std::bind(&ThriftServer::onConnection,
                                                this, std::placeholders::_1));
    }
// 
//     template <typename Processor>
//     ThriftServer(const boost::shared_ptr<Processor>& processor,
//                     const boost::shared_ptr<TTransportFactory>& transportFactory,
//                     const boost::shared_ptr<TProtocolFactory>& protocolFactory,
//                     evpp::EventLoop* eventloop,
//                     const std::string& listen_addr/*ip:port*/,
//                     const std::string& name,
//                     uint32_t thread_num)
//         : TServer(processor),
//         server_(eventloop, addr, name, thread_num),
//         numWorkerThreads_(thread_num),
//         workerThreadPool_(eventloop, thread_num) {
//         server_.SetConnectionCallback(std::bind(&ThriftServer::onConnection,
//                                                 this, std::placeholders::_1));
//         setInputTransportFactory(transportFactory);
//         setOutputTransportFactory(transportFactory);
//         setInputProtocolFactory(protocolFactory);
//         setOutputProtocolFactory(protocolFactory);
//     }
// 
//     template <typename ProcessorFactory>
//     ThriftServer(const boost::shared_ptr<ProcessorFactory>& processorFactory,
//                     const boost::shared_ptr<TTransportFactory>& inputTransportFactory,
//                     const boost::shared_ptr<TTransportFactory>& outputTransportFactory,
//                     const boost::shared_ptr<TProtocolFactory>& inputProtocolFactory,
//                     const boost::shared_ptr<TProtocolFactory>& outputProtocolFactory,
//                     evpp::EventLoop* eventloop,
//                     const std::string& listen_addr/*ip:port*/,
//                     const std::string& name,
//                     uint32_t thread_num)
//         : TServer(processorFactory),
//         server_(eventloop, addr, name, thread_num),
//         numWorkerThreads_(thread_num),
//         workerThreadPool_(eventloop, thread_num) {
//         server_.SetConnectionCallback(std::bind(&ThriftServer::onConnection,
//                                                 this, std::placeholders::_1));
//         setInputTransportFactory(inputTransportFactory);
//         setOutputTransportFactory(outputTransportFactory);
//         setInputProtocolFactory(inputProtocolFactory);
//         setOutputProtocolFactory(outputProtocolFactory);
//     }
// 
//     template <typename Processor>
//     ThriftServer(const boost::shared_ptr<Processor>& processor,
//                     const boost::shared_ptr<TTransportFactory>& inputTransportFactory,
//                     const boost::shared_ptr<TTransportFactory>& outputTransportFactory,
//                     const boost::shared_ptr<TProtocolFactory>& inputProtocolFactory,
//                     const boost::shared_ptr<TProtocolFactory>& outputProtocolFactory,
//                     evpp::EventLoop* eventloop,
//                     const std::string& listen_addr/*ip:port*/,
//                     const std::string& name,
//                     uint32_t thread_num)
//         : TServer(processor),
//         server_(eventloop, addr, name, thread_num),
//         numWorkerThreads_(thread_num),
//         workerThreadPool_(eventloop, thread_num) {
//         server_.SetConnectionCallback(std::bind(&ThriftServer::onConnection,
//                                                 this, std::placeholders::_1));
//         setInputTransportFactory(inputTransportFactory);
//         setOutputTransportFactory(outputTransportFactory);
//         setInputProtocolFactory(inputProtocolFactory);
//         setOutputProtocolFactory(outputProtocolFactory);
//     }

    virtual ~ThriftServer();

    void start();

    void stop();

    evpp::EventLoopThreadPool& workerThreadPool() {
        return workerThreadPool_;
    }

    bool isWorkerThreadPoolProcessing() const {
        return numWorkerThreads_ != 0;
    }

    evpp::TCPServer* server() {
        return &server_;
    }

private:
    friend class ThriftConnection;

    void onConnection(const evpp::TCPConnPtr& conn);
    void OnMessage(const evpp::TCPConnPtr& conn,
                   evpp::Buffer* buffer);
private:
    evpp::TCPServer server_;
    int numWorkerThreads_;
    evpp::EventLoopThreadPool workerThreadPool_;
    std::mutex mutex_;
    std::map<std::string, ThriftConnectionPtr> conns_;
}; // ThriftServer

}