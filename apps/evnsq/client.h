#pragma once

#include <string>
#include <vector>
#include <memory>
#include <atomic>
#include <map>

#include "evnsq_export.h"
#include "option.h"
#include "message.h"
#include "nsq_conn.h"

namespace evpp {
namespace httpc {
class Request;
class Response;
}
}

namespace evnsq {
class NSQConn;
typedef std::shared_ptr<NSQConn> ConnPtr;

// A Client represents a producer or consumer who holds several NSQConns with a cluster of NSQDs
class EVNSQ_EXPORT Client {
public:
    enum Type {
        kNone = 0,
        kConsumer = 1,
        kProducer = 2,
    };
    typedef std::function<void()> CloseCallback;
public:
    void ConnectToNSQD(const std::string& tcp_addr/*host:port*/);
    void ConnectToNSQDs(const std::string& tcp_addrs/*host1:port1,host2:port2*/);
    void ConnectToNSQDs(const std::vector<std::string>& tcp_addrs/*host:port*/);
    void ConnectToLoopupd(const std::string& lookupd_url/*http://127.0.0.1:4161/lookup?topic=test*/);
    void ConnectToLoopupds(const std::string& lookupd_urls/*http://192.168.0.5:4161/lookup?topic=test,http://192.168.0.6:4161/lookup?topic=test*/);
    void Close();

    void SetMessageCallback(const MessageCallback& cb) {
        msg_fn_ = cb;
    }
    void SetCloseCallback(const CloseCallback& cb) {
        close_fn_ = cb;
    }
    bool IsProducer() const {
        return type_ == kProducer;
    }
    evpp::EventLoop* loop() const {
        return loop_;
    }

protected:
    Client(evpp::EventLoop* loop, Type t, const Option& ops);
    virtual ~Client();
    void HandleLoopkupdHTTPResponse(
        const std::shared_ptr<evpp::httpc::Response>& response,
        const std::shared_ptr<evpp::httpc::Request>& request);
    void OnConnection(const ConnPtr& conn);
    void set_topic(const std::string& t) { topic_ = t; }
    void set_channel(const std::string& c) { channel_ = c; }
private:
    bool IsKnownNSQDAddress(const std::string& addr) const;
    void MoveToConnectingList(const ConnPtr& conn);
protected:
    evpp::EventLoop* loop_;
    Type type_;
    Option option_;
    std::string topic_;
    std::string channel_;
    std::map<std::string/*NSQD address "host:port"*/, ConnPtr> connecting_conns_; // The TCP connections which are connecting to NSQDs
    std::vector<ConnPtr> conns_; // The TCP connections which has established the connection with NSQDs
    MessageCallback msg_fn_;
    CloseCallback close_fn_;

    typedef std::function<void(NSQConn*)> ReadyToPublishCallback;
    ReadyToPublishCallback ready_to_publish_fn_;
};
}

