#pragma once

#include "evpp/inner_pre.h"

#include "evpp/timestamp.h"

namespace evpp {
    class Buffer;
    class TCPConn;

    typedef std::shared_ptr<TCPConn> TCPConnPtr;
    typedef std::function<void()> TimerCallback;

    // ���ӳɹ������ӶϿ�������ʧ�ܵ��¼���������øûص�
    typedef std::function<void(const TCPConnPtr&)> ConnectionCallback;


    typedef std::function<void(const TCPConnPtr&)> CloseCallback;
    typedef std::function<void(const TCPConnPtr&)> WriteCompleteCallback;
    typedef std::function<void(const TCPConnPtr&, size_t)> HighWaterMarkCallback;

    typedef std::function<void(const TCPConnPtr&, Buffer*, Timestamp)> MessageCallback;
}