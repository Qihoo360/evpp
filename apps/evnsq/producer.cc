#include "producer.h"

#include <evpp/event_loop.h>
#include "command.h"
#include "conn.h"

namespace evnsq {

    Producer::Producer(evpp::EventLoop* loop, const Option& ops)
        : Client(loop, kProducer, "", "", ops) {
        conn_ = conns_.end();
    }

    Producer::~Producer() {
    }

    void Producer::Publish(const std::string& topic, const std::string& msg) {
        assert(loop_->IsInLoopThread());
        if (conn_ == conns_.end()) {
            conn_ = conns_.begin();
        }
        assert(conn_ != conns_.end());
        assert(conn_->second->IsReady());
        Command c;
        c.Publish(topic, msg);
        conn_->second->WriteCommand(c); //TODO need to process the response code 'OK'
        ++conn_;
    }
}