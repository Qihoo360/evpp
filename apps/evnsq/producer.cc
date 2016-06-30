#include "producer.h"

#include <evpp/event_loop.h>
#include "command.h"
#include "conn.h"

namespace evnsq {

    Producer::Producer(evpp::EventLoop* loop, const std::string& topic, const std::string& channel, const Option& ops) 
        : Client(loop, kProducer, topic, channel, ops) {
        conn_ = conns_.end();
    }

    Producer::~Producer() {
    }

    void Producer::Publish(const std::string& msg) {
        assert(loop_->IsInLoopThread());
        if (conn_ == conns_.end()) {
            conn_ = conns_.begin();
        }
        assert(conn_ != conns_.end());
        assert(conn_->second->IsReady());
        Command c;
        c.Publish(topic_, msg);
        conn_->second->WriteCommand(c); //TODO need to process response code 'OK'
        ++conn_;
    }
}