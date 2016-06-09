#pragma once

#include "evpp/inner_pre.h"

#include "evpp/base/timestamp.h"

namespace evpp {
    class Buffer;
    class TCPConn;

    typedef xstd::shared_ptr<TCPConn> TcpConnectionPtr;
    typedef xstd::function<void()> TimerCallback;
    typedef xstd::function<void(const TcpConnectionPtr&)> ConnectionCallback;
    typedef xstd::function<void(const TcpConnectionPtr&)> CloseCallback;
    typedef xstd::function<void(const TcpConnectionPtr&)> WriteCompleteCallback;
    typedef xstd::function<void(const TcpConnectionPtr&, size_t)> HighWaterMarkCallback;

    // the data has been read to (buf, len)
    typedef xstd::function<void(const TcpConnectionPtr&,
                                Buffer*,
                                base::Timestamp)> MessageCallback;
}