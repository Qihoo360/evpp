#pragma once

#include "evpp/inner_pre.h"

#include "evpp/base/timestamp.h"

namespace evpp {
    class Buffer;
    class TCPConn;

    typedef xstd::shared_ptr<TCPConn> TCPConnPtr;
    typedef xstd::function<void()> TimerCallback;
    typedef xstd::function<void(const TCPConnPtr&)> ConnectionCallback;
    typedef xstd::function<void(const TCPConnPtr&)> CloseCallback;
    typedef xstd::function<void(const TCPConnPtr&)> WriteCompleteCallback;
    typedef xstd::function<void(const TCPConnPtr&, size_t)> HighWaterMarkCallback;

    typedef xstd::function<void(const TCPConnPtr&, Buffer*, base::Timestamp)> MessageCallback;
}