#pragma once

#include <string>

#include "evnsq_export.h"

#include <evpp/duration.h>

namespace evnsq {

// Option is a struct of NSQ options
struct EVNSQ_EXPORT Option {
public:
    Option();
    std::string ToJSON() const;

public:
    evpp::Duration dial_timeout = evpp::Duration(1.0);

    // Deadlines for network reads and writes
    evpp::Duration read_timeout = evpp::Duration(60.0); // min:"100ms" max:"5m"
    evpp::Duration write_timeout = evpp::Duration(1.0); // min:"100ms" max:"5m"

    // The server-side message timeout for messages delivered to this client
    // min:"0"
    evpp::Duration msg_timeout = evpp::Duration(0);

    // Identifiers sent to nsqd representing this client
    // UserAgent is in the spirit of HTTP (default: "<client_library_name>/<version>")
    std::string client_id = "evnsq"; // (defaults: short hostname)
    std::string hostname = "evnsq.localhost.com";
    std::string user_agent = "evnsq/1.0";

    // Duration of time between heartbeats. This must be less than read_timeout
    evpp::Duration heartbeat_interval = evpp::Duration(30.0);

    // Duration of interval time to query nsqlookupd
    evpp::Duration query_nsqlookupd_interval = evpp::Duration(30.0);

    //authorization
    std::string auth_secret;

    bool feature_negotiation = true;

    // Integer percentage to sample the channel (requires nsqd 0.2.25+)
    // min:"0" max : "99"
    int sample_rate = 0;
};
}
