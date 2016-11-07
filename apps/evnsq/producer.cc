#include "producer.h"

#include <evpp/event_loop.h>
#include "command.h"
#include "conn.h"

namespace evnsq {

Producer::Producer(evpp::EventLoop* loop, const Option& ops)
    : Client(loop, kProducer, "", "", ops)
    , wait_ack_count_(0)
    , published_count_(0)
    , published_ok_count_(0)
    , published_failed_count_(0)
    , hwm_triggered_(false)
    , high_water_mark_(kDefaultHighWaterMark) {
    conn_ = conns_.end();
    ready_to_publish_fn_ = std::bind(&Producer::OnReady, this, std::placeholders::_1);
}

Producer::~Producer() {}

void Producer::Publish(const std::string& topic, const std::string& msg) {
    Command* cmd = new Command;
    cmd->Publish(topic, msg);
    Publish(cmd);
}

void Producer::MultiPublish(const std::string& topic, const std::vector<std::string>& messages) {
    if (messages.empty()) {
        return;
    }

    if (messages.size() == 1) {
        Publish(topic, messages[0]);
        return;
    }

    Command* cmd = new Command;
    cmd->MultiPublish(topic, messages);
    Publish(cmd);
}

void Producer::Publish(Command* cmd) {
    if (loop_->IsInLoopThread()) {
        PublishInLoop(cmd);
    } else {
        loop_->RunInLoop(std::bind(&Producer::PublishInLoop, this, cmd));
    }
}

void Producer::SetHighWaterMarkCallback(const HighWaterMarkCallback& cb, size_t mark) {
    high_water_mark_fn_ = cb;
    high_water_mark_ = mark;
}

void Producer::PublishInLoop(Command* c) {
    if (wait_ack_count_ > kDefaultHighWaterMark * conns_.size()) {
        LOG_WARN << "Too many messages are waiting a response ACK. Please try again later";
        hwm_triggered_ = true;

        if (high_water_mark_fn_) {
            high_water_mark_fn_(this, wait_ack_count_);
        }

        return;
    }

    assert(loop_->IsInLoopThread());
    auto conn = GetNextConn();
    if (!conn.get()) {
        //TODO
        LOG_ERROR << "No available NSQD to use.";
        return;
    }
    conn->WriteCommand(c);
    published_count_++;
    PushWaitACKCommand(conn.get(), c);
    LOG_INFO << "Publish a message, command=" << c;
}

void Producer::OnReady(Conn* conn) {
    conn->SetPublishResponseCallback(std::bind(&Producer::OnPublishResponse, this, conn, std::placeholders::_1, std::placeholders::_2));

    // Only the first successful connection to NSQD can trigger this callback.
    if (ready_fn_ && conns_.size() == 1) {
        ready_fn_();
    }
}

void Producer::OnPublishResponse(Conn* conn, const char* d, size_t len) {
    Command* c = PopWaitACKCommand(conn);

    if (len == 2 && d[0] == 'O' && d[1] == 'K') {
        LOG_INFO << "Get a PublishResponse message OK, command=" << c;
        published_ok_count_++;
        delete c;

        if (hwm_triggered_ && wait_ack_count_ < kDefaultHighWaterMark * conns_.size() / 2) {
            LOG_TRACE << "We can publish more data now.";
            hwm_triggered_ = false;

            if (ready_fn_) {
                ready_fn_();
            }
        }

        return;
    }

    published_failed_count_++;
    LOG_ERROR << "Publish command " << c << " failed : [" << std::string(d, len) << "]. Try again.";
    conn_->second->WriteCommand(c);
    PushWaitACKCommand(conn_->second.get(), c);
}

Command* Producer::PopWaitACKCommand(Conn* conn) {
    CommandList& cl = wait_ack_[conn];
    assert(!cl.first.empty());
    Command* c = *cl.first.begin();
    cl.first.pop_front();
    cl.second--;
    wait_ack_count_--;
    assert(cl.first.size() == cl.second);
    return c;
}

void Producer::PushWaitACKCommand(Conn* conn, Command* cmd) {
    CommandList& cl = wait_ack_[conn];
    cl.first.push_back(cmd);
    cl.second++;
    wait_ack_count_++;
    assert(cl.first.size() == cl.second);
}

ConnPtr Producer::GetNextConn() {
    if (conns_.empty()) {
        return ConnPtr();
    }

    // TODO FIXME : this iterator conn_ may be invalid when there is some connection broken up with NSQD
    if (conn_ == conns_.end()) {
        conn_ = conns_.begin();
    }
    assert(conn_ != conns_.end());
    assert(conn_->second->IsReady());
    auto c = conn_->second;
    ++conn_; // Using next Conn
    return c;
}

}