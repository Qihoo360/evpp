#pragma once

#include <string>
#include <memory>
#include <atomic>
#include <map>

#include "evnsq_export.h"
#include "option.h"
#include "message.h"
#include "conn.h"

namespace evpp {
namespace httpc {
class Request;
class Response;
}
}

namespace evnsq {
class Conn;
typedef std::shared_ptr<Conn> ConnPtr;

class EVNSQ_EXPORT Client {
public:
    enum Type {
        kNone = 0,
        kConsumer = 1,
        kProducer = 2,
    };

    virtual ~Client();
    void ConnectToNSQD(const std::string& tcp_addr/*host:port*/);
    void ConnectToNSQDs(const std::string& tcp_addrs/*host1:port1,host2:port2*/);
    void ConnectToLoopupd(const std::string& lookupd_url/*http://127.0.0.1:4161/lookup?topic=test*/);
    void ConnectToLoopupds(const std::string& lookupd_urls/*http://192.168.0.5:4161/lookup?topic=test,http://192.168.0.6:4161/lookup?topic=test*/);
    void SetMessageCallback(const MessageCallback& cb) {
        msg_fn_ = cb;
    }
    bool IsProducer() const {
        return type_ == kProducer;
    }
    evpp::EventLoop* loop() const {
        return loop_;
    }
protected:
    Client(evpp::EventLoop* loop, Type t, const std::string& topic, const std::string& channel, const Option& ops);
    void HandleLoopkupdHTTPResponse(
        const std::shared_ptr<evpp::httpc::Response>& response,
        const std::shared_ptr<evpp::httpc::Request>& request);
    void OnConnection(const ConnPtr& conn);

protected:
    evpp::EventLoop* loop_;
    Type type_;
    Option option_;
    std::string topic_;
    std::string channel_;
    std::map<std::string/*NSQD address "host:port"*/, ConnPtr> connecting_conns_; // The TCP connections which are connecting to NSQDs
    std::map<std::string/*NSQD address "host:port"*/, ConnPtr> conns_; // The TCP connections which has established the connection with NSQDs
    MessageCallback msg_fn_;

    typedef std::function<void(Conn*)> ReadyToPublishCallback;
    ReadyToPublishCallback ready_to_publish_fn_;
};
}

