#pragma once



#include <evpp/tcp_conn.h>

#include <thrift/protocol/TProtocol.h>
#include <thrift/transport/TBufferTransports.h>
#include <thrift/transport/TTransportUtils.h>

namespace evthrift {

using apache::thrift::TProcessor;
using apache::thrift::protocol::TProtocol;
using apache::thrift::transport::TMemoryBuffer;
using apache::thrift::transport::TNullTransport;
using apache::thrift::transport::TTransport;
using apache::thrift::transport::TTransportException;

class ThriftServer;

class ThriftConn : public std::enable_shared_from_this<ThriftConn> {
public:
    enum State {
        kExpectFrameSize,
        kExpectFrame
    };

    ThriftConn(ThriftServer* server, const evpp::TCPConnPtr& conn);

private:
    friend class ThriftServer;
    void OnMessage(const evpp::TCPConnPtr& conn,
                   evpp::Buffer* buffer);

    void Process();

    void Close();

private:
    ThriftServer* server_;
    evpp::TCPConnPtr conn_;

    boost::shared_ptr<TNullTransport> nullTransport_;

    boost::shared_ptr<TMemoryBuffer> inputTransport_;
    boost::shared_ptr<TMemoryBuffer> outputTransport_;

    boost::shared_ptr<TTransport> factoryInputTransport_;
    boost::shared_ptr<TTransport> factoryOutputTransport_;

    boost::shared_ptr<TProtocol> inputProtocol_;
    boost::shared_ptr<TProtocol> outputProtocol_;

    boost::shared_ptr<TProcessor> processor_;

    enum State state_;
    uint32_t frameSize_;
};

typedef std::shared_ptr<ThriftConn> ThriftConnectionPtr;

}
