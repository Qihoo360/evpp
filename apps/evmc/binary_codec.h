#pragma once

#include <stdint.h>

#include <memcached/protocol_binary.h>

#include "evpp/buffer.h"
#include "evpp/tcp_conn.h"
#include "evpp/timestamp.h"

#include "command.h"

namespace evmc {

class MemcacheClient;

class BinaryCodec {
public:
    explicit BinaryCodec(MemcacheClient* memc_client) : memc_client_(memc_client) {}

    void OnCodecMessage(const evpp::TCPConnPtr& conn,
                        evpp::Buffer* buf);

private:
    // noncopyable
    BinaryCodec(const BinaryCodec&);
    const BinaryCodec& operator=(const BinaryCodec&);

    void RecvMultiGetData(evpp::Buffer* buf, const std::size_t size, CommandPtr& cmd);
    void RecvGetData(const void* data, CommandPtr& cmd);
    void OnResponsePacket(evpp::Buffer* buf, const std::size_t size);
    uint32_t OnRecvPrefixGetData(const char*   buf, EvmcCode& code
                                 , std::map<std::string, std::map<std::string, std::string>>& results);
    void RecvPrefixGetData(const evpp::Buffer* const buf, const std::size_t size, CommandPtr& cmd);
private:
    // TODO : 若使用智能指针，要处理循环引用. client的回调中引用了codec
    MemcacheClient* memc_client_;
};

}

