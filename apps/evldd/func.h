#pragma once 

#include "message.h"

namespace evpp {
    class EventLoop;
}

namespace evldd {

typedef std::function<const Message& msg>                   LddMessgageRetCallback;

typedef std::function<evpp::EventLoop* loop, 
        const MessagePtr& msg, LddMessgageRetCallback cb>   LddMessgageCallback;


}
