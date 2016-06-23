#include "option.h"

#include <rapidjson/rapidjson.h>
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
namespace evnsq {

    Option::Option() {
        initialized = true;
        dial_timeout = evpp::Duration(1.0);
        read_timeout = evpp::Duration(60.0);
        write_timeout = evpp::Duration(1.0);

        client_id = "evnsq";
        hostname = "evnsq.localhost.com";
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
        rapidjson::StringBuffer buffer;
        rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
        doc.Accept(writer);
        return std::string(buffer.GetString(), buffer.GetSize());
    }
}