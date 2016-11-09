#include "consumer.h"

#include <evpp/event_loop.h>
#include <evpp/tcp_client.h>
#include <evpp/tcp_conn.h>
#include <evpp/httpc/request.h>
#include <evpp/httpc/response.h>
#include <evpp/httpc/conn.h>

#include <rapidjson/document.h>

#include "command.h"
#include "option.h"

namespace evnsq {
Consumer::Consumer(evpp::EventLoop* l, const std::string& topic, const std::string& channel, const Option& ops)
    : Client(l, kConsumer, ops) {
    set_topic(topic);
    set_channel(channel);
}

Consumer::~Consumer() {
}

}
