#pragma once

#include <string>
#include <vector>
#include <memory>
#include <atomic>
#include <map>
#include <set>

#include "evnsq/config.h"
#include "option.h"
#include "message.h"
#include "nsq_conn.h"

#include "evpp/invoke_timer.h"

namespace evpp {
namespace httpc {
class Request;
class Response;
}
}

namespace evnsq {

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
    // Connect to the cluster of NSQDs directly
    void ConnectToNSQD(const std::string& tcp_addr/*host:port*/);
    void ConnectToNSQDs(const std::string& tcp_addrs/*host1:port1,host2:port2,host3:port3*/);
    void ConnectToNSQDs(const std::vector<std::string>& tcp_addrs/*host:port*/);

    // Connect to nsqlookupd(s) and get the NSQDs' addresses, and then connect to NSQDs
    void ConnectToLookupd(const std::string& lookupd_url/*http://127.0.0.1:4161/lookup?topic=test*/);
    void ConnectToLookupds(const std::string& lookupd_urls/*http://192.168.0.5:4161/lookup?topic=test1,http://192.168.0.6:4161/lookup?topic=test2,http://192.168.0.7:4161/nodes*/);

    // Close the connections with NSQDs
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

    // @return true if it is ready to produce messages to NSQD or consume messages from NSQD
    bool IsReady() const;
protected:
    Client(evpp::EventLoop* loop, Type t, const Option& ops);
    virtual ~Client();
    void HandleLoopkupdHTTPResponse(
        const std::shared_ptr<evpp::httpc::Response>& response,
        const std::shared_ptr<evpp::httpc::Request>& request);
    void OnConnection(const NSQConnPtr& conn);
    void set_topic(const std::string& t) {
        topic_ = t;
    }
    void set_channel(const std::string& c) {
        channel_ = c;
    }
    bool closing() const {
        return closing_;
    }
private:
    bool IsKnownNSQDAddress(const std::string& addr) const;
    void MoveToConnectingList(const NSQConnPtr& conn);
protected:
    evpp::EventLoop* loop_;
    Type type_;
    Option option_;
    std::string topic_;
    std::string channel_;
    std::map<std::string/*NSQD address "host:port"*/, NSQConnPtr> connecting_conns_; // The TCP connections which are connecting to NSQDs
    std::vector<NSQConnPtr> conns_; // The TCP connections which has established the connection with NSQDs
    MessageCallback msg_fn_;
    CloseCallback close_fn_;
    std::vector<evpp::InvokeTimerPtr> lookupd_timers_;
    bool closing_;

    typedef std::function<void(NSQConn*)> ReadyToPublishCallback;
    ReadyToPublishCallback ready_to_publish_fn_;
};
}

