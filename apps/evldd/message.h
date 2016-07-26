#pragma once

namespace evldd {

typedef int16_t     Code;
typedef int32_t     MessageId;
typedef uint16_t    MessageType;
typedef uint16_t    PayloadType;

typedef std::map<int, std::string>  ExtraMap;

struct Message {
    MessageType message_type;
    PayloadType payload_type;
    ExtraMap    map;
    std::string body;
}

typedef std::shared_ptr<Message> MessagePtr;

}
