#include "thrift_conn.h"

#include <thrift/transport/TTransportException.h>

#include "thrift_server.h"

namespace evthrift {

ThriftConn::ThriftConn(ThriftServer* server,
                                    const evpp::TCPConnPtr& conn)
    : server_(server),
    conn_(conn),
    state_(kExpectFrameSize),
    frameSize_(0) {

    nullTransport_.reset(new TNullTransport());
    inputTransport_.reset(new TMemoryBuffer(NULL, 0));
    outputTransport_.reset(new TMemoryBuffer());

    factoryInputTransport_ = server_->getInputTransportFactory()->getTransport(inputTransport_);
    factoryOutputTransport_ = server_->getOutputTransportFactory()->getTransport(outputTransport_);

    inputProtocol_ = server_->getInputProtocolFactory()->getProtocol(factoryInputTransport_);
    outputProtocol_ = server_->getOutputProtocolFactory()->getProtocol(factoryOutputTransport_);

    processor_ = server_->getProcessor(inputProtocol_, outputProtocol_, nullTransport_);
}

void ThriftConn::OnMessage(const evpp::TCPConnPtr& conn,
                                    evpp::Buffer* buffer) {
    bool more = true;
    while (more) {
        if (state_ == kExpectFrameSize) {
            if (buffer->size() >= 4) {
                frameSize_ = static_cast<uint32_t>(buffer->ReadInt32());
                state_ = kExpectFrame;
            } else {
                more = false;
            }
        } else if (state_ == kExpectFrame) {
            if (buffer->size() >= frameSize_) {
                uint8_t* buf = reinterpret_cast<uint8_t*>((const_cast<char*>(buffer->data())));

                inputTransport_->resetBuffer(buf, frameSize_, TMemoryBuffer::COPY);
                outputTransport_->resetBuffer();
                outputTransport_->getWritePtr(4);
                outputTransport_->wroteBytes(4);

                Process();

                buffer->Retrieve(frameSize_);
                state_ = kExpectFrameSize;
            } else {
                more = false;
            }
        }
    }
}

void ThriftConn::Process() {
    try {
        processor_->process(inputProtocol_, outputProtocol_, NULL);

        uint8_t* buf;
        uint32_t size;
        outputTransport_->getBuffer(&buf, &size);

        assert(size >= 4);
        uint32_t frameSize = static_cast<uint32_t>(htonl(size - 4));
        memcpy(buf, &frameSize, 4);

        conn_->Send(buf, size);
    } catch (const TTransportException& ex) {
        LOG_ERROR << "ThriftServer TTransportException: " << ex.what();
        Close();
    } catch (const std::exception& ex) {
        LOG_ERROR << "ThriftServer std::exception: " << ex.what();
        Close();
    } catch (...) {
        LOG_ERROR << "ThriftServer unknown exception";
        Close();
    }
}

void ThriftConn::Close() {
    nullTransport_->close();
    factoryInputTransport_->close();
    factoryOutputTransport_->close();
}

}