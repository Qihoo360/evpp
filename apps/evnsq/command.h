#pragma once

#include <string>
#include <vector>
#include <memory>

#include <evpp/buffer.h>
#include <evpp/duration.h>
#include <evpp/utility.h>

#include "evnsq_export.h"
#include "message.h"

namespace evnsq {
class EVNSQ_EXPORT Command {
public:
    // Identify sets a Command to provide information about the client.
    // After connecting, it is generally the first message sent.
    // See http://nsq.io/clients/tcp_protocol_spec.html#identify for information
    // on the supported options
    void Identify(const std::string& js) {
        name_ = "IDENTIFY";
        body_.push_back(js);
    }

    // Auth sends credentials for authentication
    //
    // After `Identify`, this is usually the first message sent, if auth is used.
    void Auth(const  std::string& secret) {
        name_ = "AUTH";
        body_.push_back(secret);
    }

    // Register sets a new Command to add a topic/channel for the connected nsqd
    void Register(const std::string& topic, const std::string& channel) {
        assert(!topic.empty());
        name_ = "REGISTER";
        params_.push_back(topic);

        if (!channel.empty()) {
            params_.push_back(channel);
        }
    }

    // UnRegister sets a new Command to remove a topic/channel for the connected nsqd
    void UnRegister(const std::string& topic, const std::string& channel) {
        assert(!topic.empty());
        name_ = "UNREGISTER";
        params_.push_back(topic);

        if (!channel.empty()) {
            params_.push_back(channel);
        }
    }

    // Ping sets a new Command to keep-alive the state of all the
    // announced topic/channels for a given client
    void Ping() {
        name_ = "PING";
    }

    // Publish sets a new Command to write a message to a given topic
    void Publish(const std::string& topic, const std::string& body) {
        assert(!topic.empty());
        name_ = "PUB";
        params_.push_back(topic);
        body_.push_back(body);
    }

    void MultiPublish(const std::string& topic, const std::vector<std::string>& messages) {
        assert(!topic.empty());
        assert(messages.size() > 1);
        name_ = "MPUB";
        params_.push_back(topic);
        body_ = messages;
    }

    // Subscribe sets a new Command to subscribe to the given topic/channel
    void Subscribe(const std::string& topic, const std::string& channel) {
        assert(!topic.empty());
        assert(!channel.empty());
        name_ = "SUB";
        params_.push_back(topic);
        params_.push_back(channel);
    }

    // Ready sets a new Command to specify
    // the number of messages a client is willing to receive
    void Ready(int count) {
        name_ = "RDY";
        params_.push_back(std::to_string(count));
    }

    // Finish sets a new Command to indicate that
    // a given message (by id) has been processed successfully
    void Finish(const std::string& id) {
        assert(id.size() == kMessageIDLen);
        name_ = "FIN";
        params_.push_back(id);
    }

    // Requeue sets a new Command to indicate that
    // a given message (by id) should be requeued after the given delay
    // NOTE: a delay of 0 indicates immediate requeue
    void Requeue(const std::string& id, evpp::Duration delay) {
        assert(id.size() == kMessageIDLen);
        name_ = "REQ";
        params_.push_back(id);
        params_.push_back(std::to_string(int(delay.Milliseconds())));
    }

    // Touch sets a new Command to reset the timeout for
    // a given message (by id)
    void Touch(const std::string& id) {
        params_.push_back(id);
        name_ = "TOUCH";
    }

    // StartClose sets a new Command to indicate that the
    // client would like to start a close cycle.  nsqd will no longer
    // send messages to a client in this state and the client is expected
    // finish pending messages and close the connection
    void StartClose() {
        name_ = "CLS";
    }

    // Nop sets a new Command that has no effect server side.
    // Commonly used to respond to heartbeats
    void Nop() {
        name_ = "NOP";
    }

    // Serializes the Command to the supplied Buffer
    void WriteTo(evpp::Buffer* buf) const;
    void Reset() {
        name_.clear();
        params_.clear();
        body_.clear();
    }
private:
    std::string name_;
    std::vector<std::string> params_;
    std::vector<std::string> body_;
};
}

