#pragma once

#include<map>
#include<string>

namespace evldd {

typedef int16_t     Code;
typedef int32_t     MessageId;
typedef uint16_t    MessageType;
typedef uint16_t    PayloadType;

typedef std::map<int, std::string>  ExtraMap;

struct Message {
    MessageType message_type;
    PayloadType payload_type;
    std::string body;
    ExtraMap*   extras;

    Message();
};

typedef std::shared_ptr<Message> MessagePtr;

}
