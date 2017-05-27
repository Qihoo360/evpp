#pragma once

#include "evnsq/config.h"
#include <evpp/buffer.h>
#include <evpp/slice.h>

namespace evnsq {

enum { kMessageIDLen = 16 };
enum { kFrameTypeResponse = 0, kFrameTypeError = 1, kFrameTypeMessage = 2, };

struct Message {
    int64_t timestamp_ns; // nanosecond
    uint16_t attempts;
    std::string id; // with length equal to kMessageIDLen
    evpp::Slice body;

    // Decode deserializes data (as Buffer*) to this message
    // message format:
    // [x][x][x][x][x][x][x][x][x][x][x][x][x][x][x][x][x][x][x][x][x][x][x][x][x][x][x][x][x][x]...
    // |       (int64)        ||    ||      (hex string encoded in ASCII)           || (binary)
    // |       8-byte         ||    ||                 16-byte                      || N-byte
    // ------------------------------------------------------------------------------------------...
    //   nanosecond timestamp    ^^                   message ID                       message body
    //                        (uint16)
    //                         2-byte
    //                        attempts
    bool Decode(size_t message_len, evpp::Buffer* buf) {
        assert(buf->size() >= message_len);
        if (buf->size() < message_len) {
            return false;
        }
        timestamp_ns = buf->ReadInt64();
        attempts = buf->ReadInt16();
        id = buf->NextString(kMessageIDLen);
        size_t body_len = message_len - sizeof(timestamp_ns) - sizeof(attempts) - kMessageIDLen;
        body = evpp::Slice(buf->data(), body_len); // No copy
        buf->Retrieve(body_len);
        return true;
    }
};


// MessageCallback is the message processing interface for Consumer
//
// Implement this interface for handlers that return whether or not message
// processing completed successfully.
//
// When the return value is 0 Consumer will automatically handle FINishing.
//
// When the returned value is non-zero Consumer will automatically handle REQueing.
typedef std::function<int(const Message*)> MessageCallback;
}