#include "producer.h"

#include <evpp/event_loop.h>
#include "command.h"
#include "nsq_conn.h"

namespace evnsq {

Producer::Producer(evpp::EventLoop* l, const Option& ops)
    : Client(l, kProducer, ops)
    , current_conn_index_(0)
    , published_count_(0)
    , published_ok_count_(0)
    , published_failed_count_(0)
    , hwm_triggered_(false)
    , high_water_mark_(kDefaultHighWaterMark) {
    // TODO Remember to remove these callbacks from EventLoop when stopping this Producer
    ready_to_publish_fn_ = std::bind(&Producer::OnReady, this, std::placeholders::_1);
}

Producer::~Producer() {}

bool Producer::Publish(const std::string& topic, const std::string& msg) {
    CommandPtr cmd(new Command);
    cmd->Publish(topic, msg);
    return Publish(cmd);
}

bool Producer::MultiPublish(const std::string& topic, const std::vector<std::string>& messages) {
    if (messages.empty()) {
        return true;
    }

    if (messages.size() == 1) {
        return Publish(topic, messages[0]);
    }

    CommandPtr cmd(new Command);
    cmd->MultiPublish(topic, messages);
    return Publish(cmd);
}


bool Producer::PublishBinaryCommand(evpp::Buffer* command_binary_buf) {
    if (closing()) {
        return false;
    }

    assert(loop_->IsInLoopThread());
    auto conn = GetNextConn();
    if (!conn.get()) {
        LOG_ERROR << "No available NSQD to use.";
        return false;
    }

    published_count_ += 1;
    conn->WriteBinaryCommand(command_binary_buf);
    return true;
}

bool Producer::Publish(const CommandPtr& cmd) {
    if (closing()) {
        return false;
    }

    if (conns_.empty()) {
        LOG_ERROR << "No available NSQD to use.";
        return false;
    }

    if (loop_->IsInLoopThread()) {
        return PublishInLoop(cmd);
    }
    loop_->RunInLoop(std::bind(&Producer::PublishInLoop, this, cmd));
    return true;
}

void Producer::SetHighWaterMarkCallback(const HighWaterMarkCallback& cb, size_t mark) {
    high_water_mark_fn_ = cb;
    high_water_mark_ = mark;
}

bool Producer::PublishInLoop(const CommandPtr& cmd) {
    assert(loop_->IsInLoopThread());
    auto conn = GetNextConn();
    if (!conn.get()) {
        LOG_ERROR << "No available NSQD to use.";
        return false;
    }

    // PUB will only add 1 to published_count_
    // MPUB will add size(MPUB) to published_count_
    published_count_ += cmd->body().size();
    bool rc = conn->WritePublishCommand(cmd);
    if (!rc) {
        published_failed_count_ += cmd->body().size();
    }
    return rc;
}

void Producer::OnReady(NSQConn* conn) {
    conn->SetPublishResponseCallback(std::bind(&Producer::OnPublishResponse, this, conn, std::placeholders::_1, std::placeholders::_2));

    // Only the first successful connection to NSQD can trigger this callback.
    if (ready_fn_ && conns_.size() == 1) {
        ready_fn_();
    }
}

void Producer::OnPublishResponse(NSQConn* conn, const CommandPtr& cmd, bool successfull) {
    size_t count = 1;
    if (cmd.get()) {
        count = cmd->body().size();
    }

    if (successfull) {
        published_ok_count_ += count;
    } else {
        published_failed_count_ += count;
    }
}

NSQConnPtr Producer::GetNextConn() {
    if (conns_.empty()) {
        return NSQConnPtr();
    }

    if (current_conn_index_ >= conns_.size()) {
        current_conn_index_ = 0;
    }
    auto c = conns_[current_conn_index_];
    assert(c->IsReady());
    ++current_conn_index_; // Using next Conn
    return c;
}

void Producer::PrintStats() {
    LOG_WARN << "published_count=" << published_count_
             << " published_ok_count=" << published_ok_count_
             << " published_failed_count=" << published_failed_count_;
    published_count_ = 0;
    published_ok_count_ = 0;
    published_failed_count_ = 0;
}

}
