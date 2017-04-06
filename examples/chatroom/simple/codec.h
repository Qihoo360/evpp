#pragma once

#include <evpp/tcp_conn.h>
#include <evpp/buffer.h>

class LengthHeaderCodec {
public:
    typedef std::function<void(const evpp::TCPConnPtr&,
                               const std::string& message)> StringMessageCallback;

    explicit LengthHeaderCodec(const StringMessageCallback& cb)
        : messageCallback_(cb) {}

    void OnMessage(const evpp::TCPConnPtr& conn,
                   evpp::Buffer* buf) {
        while (buf->size() >= kHeaderLen) {
            const int32_t len = buf->PeekInt32();
            if (len > 65536 || len < 0) {
                LOG_ERROR << "Invalid length " << len;
                conn->Close();
                break;
            }

            if (buf->size() >= len + kHeaderLen) {
                buf->Skip(kHeaderLen);
                std::string message(buf->NextString(len));
                messageCallback_(conn, message);
                break;
            } else {
                break;
            }
        }
    }

    void Send(evpp::TCPConnPtr conn,
              const evpp::Slice& message) {
        evpp::Buffer buf;
        buf.Append(message.data(), message.size());
        buf.PrependInt32(message.size());
        conn->Send(&buf);
    }

private:
    StringMessageCallback messageCallback_;
    const static size_t kHeaderLen = sizeof(int32_t);
};

#include "../../winmain-inl.h"
