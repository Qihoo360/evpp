#include "option.h"

#include <rapidjson/rapidjson.h>
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

namespace evnsq {

Option::Option() {
    dial_timeout = evpp::Duration(1.0);
    read_timeout = evpp::Duration(60.0);
    write_timeout = evpp::Duration(1.0);
    heartbeat_interval = evpp::Duration(30.0);
    query_nsqlookupd_interval = evpp::Duration(30.0);

    client_id = "evnsq";
    hostname = "evnsq.localhost.com"; // TODO get hostname
    user_agent = "evnsq/1.0";
}

std::string Option::ToJSON() const {
    rapidjson::Document doc;
    rapidjson::Value v;
    doc.SetObject();
    doc.AddMember("dial_timeout", rapidjson::Value(int64_t(dial_timeout.Milliseconds())), doc.GetAllocator());
    doc.AddMember("read_timeout", rapidjson::Value(int64_t(read_timeout.Milliseconds())), doc.GetAllocator());
    doc.AddMember("write_timeout", rapidjson::Value(int64_t(write_timeout.Milliseconds())), doc.GetAllocator());
    doc.AddMember("client_id", rapidjson::Value(client_id, doc.GetAllocator()), doc.GetAllocator());
    doc.AddMember("hostname", rapidjson::Value(hostname, doc.GetAllocator()), doc.GetAllocator());
    doc.AddMember("user_agent", rapidjson::Value(user_agent, doc.GetAllocator()), doc.GetAllocator());
    doc.AddMember("heartbeat_interval", rapidjson::Value(int64_t(heartbeat_interval.Milliseconds())), doc.GetAllocator());

    doc.AddMember("deflate", rapidjson::Value(false), doc.GetAllocator());
    doc.AddMember("feature_negotiation", rapidjson::Value(false), doc.GetAllocator());
    doc.AddMember("long_id", rapidjson::Value(hostname, doc.GetAllocator()), doc.GetAllocator());
    doc.AddMember("output_buffer_size", rapidjson::Value(int64_t(16384)), doc.GetAllocator());
    doc.AddMember("output_buffer_timeout", rapidjson::Value(int64_t(250)), doc.GetAllocator());

    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    doc.Accept(writer);
    return std::string(buffer.GetString(), buffer.GetSize());
}
}