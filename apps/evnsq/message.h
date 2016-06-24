#pragma once

#include "evnsq_export.h"
#include <evpp/exp.h>
#include <evpp/buffer.h>

namespace evnsq {
    enum { kMessageIDLen = 16 };
    enum { kFrameTypeResponse = 0, kFrameTypeError = 1, kFrameTypeMessage = 2, };
    struct Message {
        int64_t timestamp;
        uint16_t attempts;
        const char* id; // with length equal to kMessageIDLen
        const char* body;
        size_t body_len;

        void Decode(size_t message_len, evpp::Buffer* buf) {
            assert(buf->size() >= message_len);
            timestamp = buf->ReadInt64();
            attempts = buf->ReadInt16();
            id = buf->data(); // No copy
            body_len = message_len - sizeof(timestamp) - sizeof(attempts) - kMessageIDLen;
            body = buf->data() + kMessageIDLen; // No copy
            buf->Skip(body_len + kMessageIDLen);
        }
    };
}