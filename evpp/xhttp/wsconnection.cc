#include "wsconnection.h"

#include <stdlib.h>
#include <vector>
#include <assert.h>
#include <arpa/inet.h>
#include <string.h>
#include <endian.h>

#include "../tcp_conn.h"
#include "httprequest.h"
#include "httpresponse.h"
#include "../stringutil.h"
//#include "sockutil.h"
#include "../logging.h"

using namespace std;
using namespace std::placeholders;

namespace evpp { 

namespace xhttp { 

size_t WsConnection::ms_maxPayloadLen = 10 * 1024 * 1024;

static void dummyCallback()
{}

WsConnection::WsConnection(const TCPConnPtr& conn, const WsCallback_t& callback)
{
    //m_fd = conn->getSockFd();

    //m_conn = conn;
    m_callback = callback;

    m_payloadLen = 0;
    m_final = 0;
    m_opcode = 0;
    m_mask = 0;
    m_lastOpcode = 0;

    m_sendCallback = std::bind(&dummyCallback);
}

WsConnection::~WsConnection()
{
    //LOG_INFO("wsconnection destroyed");
}

void WsConnection::onOpen(const void* context)
{
    m_status = FrameStart;
    
    m_callback(shared_from_this(), Ws_OpenEvent, context);    
}

void WsConnection::onError()
{
    m_callback(shared_from_this(), Ws_ErrorEvent, 0);    
}

// void WsConnection::onConnEvent(const ConnectionPtr_t& conn, ConnEvent event, const void* context)
// {
//     switch(event)
//     {
//         case Conn_ReadEvent:
//             {
//                 const StackBuffer* buffer = (const StackBuffer*)context;
//                 onRead(conn, buffer->buffer, buffer->count);
//             }
//             break;    
//         case Conn_WriteCompleteEvent:
//             {
//                 m_sendCallback();
//                 m_sendCallback = std::bind(&dummyCallback);    
//             }
//             break;
//         default:
//             break;
//     }
// }

#define HANDLE(func) \
ret = func(data + readLen, count - readLen); \
readLen += (ret > 0 ? ret : 0); 

// ssize_t WsConnection::onRead(const ConnectionPtr_t& conn, const char* data, size_t count)
// {
//     size_t readLen = 0;
//     ssize_t ret = 1;

//     while(readLen < count && ret > 0)
//     {
//         switch(m_status)
//         {
//             case FrameStart:
//                 HANDLE(onFrameStart);
//             case FramePayloadLen:
//                 HANDLE(onFramePayloadLen);
//                 break;   
//             case FramePayloadLen16:
//                 HANDLE(onFramePayloadLen16);
//                 break;
//             case FramePayloadLen64:
//                 HANDLE(onFramePayloadLen64);
//                 break;
//             case FrameMaskingKey:
//                 HANDLE(onFrameMaskingKey);
//                 break;
//             case FrameData:
//                 HANDLE(onFrameData);
//                 break;
//             case FrameFinal:
//                 ret = handleFrameData(conn);
//                 readLen += (ret > 0 ? ret : 0);
//                 break;
//             default:
//                 return -1;
//                 break; 
//         }
//     }
    
//     if(ret > 0 && m_status == FrameFinal)
//     {
//         ret = handleFrameData(conn); 
//     }
    
//     if(ret < 0)
//     {
//         LOG_ERROR("onReadError");

//         m_status = FrameError;
//         m_callback(shared_from_this(), Ws_ErrorEvent, 0);
        
//         //an error occur, only to shutdown connection
//         conn->shutDown();    
//     }
    
//     return ret;
// }

ssize_t WsConnection::onFrameStart(const char* data, size_t count)
{
    m_cache.clear();

    char header = data[0];
    
    if(header & 0x70)
    {
        //reserved bits now not supported, abort
        return -1;    
    }

    m_final = header & 0x80;
    m_opcode = header & 0x0f;

    m_status = FramePayloadLen;
    return 1;    
}

ssize_t WsConnection::handleFramePayloadLen(size_t payloadLen)
{
    m_payloadLen = payloadLen;
    
    m_cache.reserve(payloadLen);
    m_cache.clear();

    if(m_payloadLen == 0)
    {
        m_status = FrameFinal;
    }
    else
    {
        m_status = isMaskFrame() ? FrameMaskingKey : FrameData;    
    }
    return 0;
}

ssize_t WsConnection::onFramePayloadLen(const char* data, size_t count)
{
    uint8_t payloadLen = (uint8_t)data[0];
    
    m_mask = payloadLen & 0x80;
    
    payloadLen &= 0x7f;
    if(isControlFrame() && payloadLen >= 126)
    {
        //control frames must have payload < 126
        return -1;    
    }
    
    if(payloadLen < 126)
    {
        handleFramePayloadLen(payloadLen);
    }
    else if(payloadLen == 126)
    {
        m_status = FramePayloadLen16;    
    }
    else if(payloadLen == 127)
    {
        m_status = FramePayloadLen64;    
    }
    else
    {
        //payload error
        return -1;    
    }

    return 1;    
}

ssize_t WsConnection::tryRead(const char* data, size_t count, size_t tryReadData)
{
    assert(m_cache.size() < tryReadData);
    size_t pendingSize = m_cache.size();
    if(pendingSize + count < tryReadData)
    {
        m_cache.append(data, count);
        return 0;    
    }

    m_cache.append(data, tryReadData - m_cache.size());

    return tryReadData - pendingSize;
}

ssize_t WsConnection::onFramePayloadLen16(const char* data, size_t count)
{
    ssize_t readLen = tryRead(data, count, 2);
    if(readLen == 0)
    {
        return readLen;    
    }
    
    uint16_t payloadLen = *(uint16_t*)m_cache.data();
    //memcpy(&payloadLen, m_cache.data(), sizeof(uint16_t));
    
    if(payloadLen > ms_maxPayloadLen)
    {
        return -1;    
    }

    payloadLen = ntohs(payloadLen); 

    handleFramePayloadLen(payloadLen);

    return readLen; 
}

ssize_t WsConnection::onFramePayloadLen64(const char* data, size_t count)
{
    ssize_t readLen = tryRead(data, count, 8);
    if(readLen == 0)
    {
        return readLen;    
    }

    uint64_t payloadLen = *(uint64_t*)m_cache.data();
    //memcpy(&payloadLen, m_cache.data(), sizeof(uint64_t));
    
    if(payloadLen > ms_maxPayloadLen)
    {
        return -1;
    }

    //todo ntohl64
    payloadLen = be64toh(payloadLen);
    
    handleFramePayloadLen(payloadLen);

    return readLen;
}

ssize_t WsConnection::onFrameMaskingKey(const char* data, size_t count)
{
    ssize_t readLen = tryRead(data, count, 4);
    if(readLen == 0)
    {
        return 0;
    }    

    memcpy(m_maskingKey, m_cache.data(), sizeof(m_maskingKey));
    
    m_cache.clear();
    
    m_status = FrameData;

    return readLen; 
}

ssize_t WsConnection::onFrameData(const char* data, size_t count)
{
    ssize_t readLen = tryRead(data, count, m_payloadLen);

    if(readLen == 0)
    {
        return 0;    
    }    

    if(isMaskFrame())
    {
        for(size_t i = 0; i < m_cache.size(); ++i)
        {
            m_cache[i] = m_cache[i] ^ m_maskingKey[i % 4];   
        }    
    }

    m_status = FrameFinal;
    return readLen;
}

// ssize_t WsConnection::handleFrameData(const ConnectionPtr_t& conn)
// {
//     uint8_t opcode = m_opcode;
//     string data = m_cache;

//     if(isControlFrame())
//     {
//         if(!isFinalFrame())
//         {
//             //control frames must not be fragmented
//             return -1;    
//         }    
//     }    
//     else if(m_opcode == 0)
//     {
//         //continuation frame
//         if(m_appData.empty())
//         {
//             //nothing to continue
//             return -1;    
//         }    

//         m_appData += m_cache;

//         if(isFinalFrame())
//         {
//             opcode = m_lastOpcode;
//             data = m_appData;
//             string().swap(m_appData);
//         }
//     }
//     else
//     {
//         //start of new data message
//         if(!m_appData.empty())
//         {
//             //can't start new message until the old one is finished
//             return -1;    
//         }    
        
//         if(isFinalFrame())
//         {
//             opcode = m_opcode;
//             data = m_cache;
//         }
//         else
//         {
//             m_lastOpcode = m_opcode;
//             m_appData = m_cache;    
//         }
//     }

//     string().swap(m_cache);

//     if(isFinalFrame())
//     {
//         if(handleMessage(conn, opcode, data) < 0)
//         {
//             return -1;    
//         }            
//     }

//     m_status = FrameStart;

//     return 0;
// }

// ssize_t WsConnection::handleMessage(const ConnectionPtr_t& conn, uint8_t opcode, const string& data)
// {
//     switch(opcode)
//     {
//         case 0x1:
//             //utf-8 data
//             m_callback(shared_from_this(), Ws_MessageEvent, (void*)&data);
//             break;    
//         case 0x2:
//             //binary data
//             m_callback(shared_from_this(), Ws_MessageEvent, (void*)&data);
//             break;
//         case 0x8:
//             //clsoe
//             m_callback(shared_from_this(), Ws_CloseEvent, 0);
            
//             conn->shutDown(500);
//             break;
//         case 0x9:
//             //ping
//             sendFrame(true, 0xA, data);
//             break;
//         case 0xA:
//             //pong
//             m_callback(shared_from_this(), Ws_PongEvent, (void*)&data);
//             break;
//         default:
//             //error
//             LOG_ERROR("invalid opcode %d", opcode);
//             return -1;
//     }    

//     return 0;
// } 

// void WsConnection::ping(const string& message)
// {
//     sendFrame(true, 0x9, message);
// }

// void WsConnection::send(const string& message)
// {
//     send(message, !isTextFrame());     
// }

// void WsConnection::send(const string& message, bool binary)
// {
//     char opcode = binary ? 0x2 : 0x1;

//     //for utf-8, we assume message is already utf-8
//     sendFrame(true, opcode, message);
// }

// void WsConnection::send(const string& message, const Callback_t& callback)
// {
//     m_sendCallback = callback;
//     send(message);    
// }

// void WsConnection::send(const string& message, bool binary, const Callback_t& callback)
// {
//     m_sendCallback = callback;
//     send(message, binary);    
// }

// void WsConnection::close()
// {
//     sendFrame(true, (char)0x8);    
// }

// void WsConnection::sendFrame(bool finalFrame, char opcode, const string& message)
// {
//     ConnectionPtr_t conn = m_conn.lock();
//     if(!conn)
//     {
//         return;    
//     }

//     string buf;

//     opcode |= (finalFrame ? 0x80 : 0x00);
    
//     buf.append((char*)&opcode, sizeof(opcode));

//     //char mask = isMaskFrame() ? 0x80 : 0x00;
//     //chrome not support mask
//     char mask = 0x00;

//     size_t payloadLen = message.size();

//     if(payloadLen < 126)
//     {
//         char payload = mask | (char)payloadLen;
//         buf.append((char*)&payload, sizeof(payload));
//     }
//     else if(payloadLen <= 0xFFFF)
//     {
//         char payload = mask | 126;
//         buf.append((char*)&payload, sizeof(payload));
//         uint16_t len = htons((uint16_t)payloadLen);

//         buf.append((char*)&len, sizeof(uint16_t));
//     }
//     else
//     {    
//         char payload = mask | 127;
//         buf.append((char*)&payload, sizeof(payload));
    
//         uint64_t len = htobe64(payloadLen);
//         buf.append((char*)&len, sizeof(uint64_t));
//     }

//     if(mask)
//     {
//         int randomKey = rand();
//         char maskingKey[4];
//         memcpy(maskingKey, &randomKey, sizeof(maskingKey));

//         buf.append(maskingKey, 4);

//         size_t pos = buf.size();
//         buf.append(message);
//         for(size_t i = 0; i < buf.size() - pos; ++i)
//         {
//             buf[i + pos] = buf[i + pos] ^ maskingKey[i % 4];     
//         }
//     }
//     else
//     {
//         buf.append(message);    
//     }

//     conn->send(buf);
// }

// void WsConnection::shutDown()
// {
//     ConnectionPtr_t conn = m_conn.lock();
//     if(conn)
//     {
//         conn->shutDown();    
//     }    
// }
}
}