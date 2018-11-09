#pragma once

#include <string>
#include <vector>
#include <stdint.h>
#include "../tcp_conn.h"
#include "../tcp_callbacks.h"
#include "xhttp.h"

namespace evpp{
    //now we wiil only support rfc6455
    //refer to tornado websocket implementation
class TCPConn;

namespace xhttp { 


class HttpRequest;

class WsConnection : public std::enable_shared_from_this<WsConnection> 
{
public:
    friend class HttpServer;
    friend class WsClient;

    WsConnection(const TCPConnPtr& conn, const WsCallback_t& callback);
    ~WsConnection();

    int getSockFd() const { return m_fd; }

    void ping(const std::string& message);
    void send(const std::string& message, bool binary);
    void send(const std::string& message);

    //callback likes HttpConnection send callback 
    void send(const std::string& message, bool binary, const Callback_t& callback);
    void send(const std::string& message, const Callback_t& callback);

    void close();

    void shutDown(); 

    static void setMaxPayloadLen(size_t len) { ms_maxPayloadLen = len; }

private:    
    //void onConnEvent(const ConnectionPtr_t& conn, ConnEvent event, const void*);

    void onOpen(const void* context);
    void onError();
            
    //ssize_t onRead(const ConnectionPtr_t& conn, const char* data, size_t count);

    bool isFinalFrame() { return m_final; }
    bool isMaskFrame() { return m_mask; }
    bool isControlFrame() { return m_opcode & 0x08; }
    bool isTextFrame() { return (m_opcode == 0) ? (m_lastOpcode == 0x1) : (m_opcode == 0x1); }
    bool isBinaryFrame() { return (m_opcode == 0) ? (m_lastOpcode == 0x2) : (m_opcode == 0x2); }
    
    ssize_t onFrameStart(const char* data, size_t count);
    ssize_t onFramePayloadLen(const char* data, size_t count);
    ssize_t onFramePayloadLen16(const char* data, size_t count);
    ssize_t onFramePayloadLen64(const char* data, size_t count);

    ssize_t onFrameMaskingKey(const char* data, size_t count);
    ssize_t onFrameData(const char* data, size_t count);

    ssize_t handleFramePayloadLen(size_t payloadLen);
    //ssize_t handleFrameData(const ConnectionPtr_t& conn);
    //ssize_t handleMessage(const ConnectionPtr_t& conn, uint8_t opcode, const std::string& message);
    ssize_t tryRead(const char* data, size_t count, size_t tryReadData);

    void sendFrame(bool finalFrame, char opcode, const std::string& message = std::string());

private:
    enum FrameStatus
    {
        FrameStart,
        FramePayloadLen,
        FramePayloadLen16,
        FramePayloadLen64,
        FrameMaskingKey,
        FrameData,
        FrameFinal,
        FrameError,
    }; 
    
    //WeakConnectionPtr_t m_conn;

    size_t m_payloadLen;

    FrameStatus m_status;

    uint8_t m_maskingKey[4];

    uint8_t m_final;
    uint8_t m_opcode;
    uint8_t m_mask;
    uint8_t m_lastOpcode;

    std::string m_cache;

    std::string m_appData;
    
    WsCallback_t m_callback;
    
    Callback_t m_sendCallback;

    int m_fd;

    static size_t ms_maxPayloadLen;
};    
}
}

