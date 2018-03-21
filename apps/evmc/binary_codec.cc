#include "binary_codec.h"

#include "memcache_client.h"

namespace evmc {

static const size_t kHeaderLen = sizeof(protocol_binary_response_header);

#define bodylen(data) ntohl(*(const long *)((const char *)data + (size_t)&(((protocol_binary_response_header*)0)->response.bodylen)))
#define keylen(data) ntohs(*(const short *)((const char *)data + (size_t)&(((protocol_binary_response_header*)0)->response.keylen)))
#define opcode(data) (*((const uint8_t *)(const char *)data + (size_t)&(((protocol_binary_response_header*)0)->response.opcode)))
#define cmdid(data) (*(const uint32_t *)((const char *)data + (size_t)&(((protocol_binary_response_header*)0)->response.opaque)))
#define cmdstatus(data) ntohs(*(const short *)((const char *)data + (size_t)&(((protocol_binary_response_header*)0)->response.status)))
#define extlen(data) (*(const uint8_t *)((const char *)data + (size_t)&(((protocol_binary_response_header*)0)->response.extlen)))

void BinaryCodec::OnCodecMessage(const evpp::TCPConnPtr& conn,
                                 evpp::Buffer* buf) {
    long bodysize = 0;
    uint8_t op = 0;
    auto size = buf->size();
    std::size_t  pos = 0;
    while ((pos + kHeaderLen) <= size) { // kHeaderLen == 24
        const void* data = buf->data() + pos;
        bodysize = bodylen(data);
        op = opcode(data);
        if ((pos + bodysize + kHeaderLen) <= size) {
            if (op == PROTOCOL_BINARY_CMD_SET || op == PROTOCOL_BINARY_CMD_DELETE
                    || op == PROTOCOL_BINARY_CMD_GET || op == PROTOCOL_BINARY_CMD_GETK
                    || op == PROTOCOL_BINARY_CMD_PGETK || op == PROTOCOL_BINARY_CMD_NOOP) {
                pos += bodysize + kHeaderLen;
                OnResponsePacket(buf, pos);
                buf->Retrieve(pos);
                pos = 0;
                size = buf->size();
                continue;
            }
        } else {
            LOG_TRACE << "need recv more data";
            break;
        }
        pos += bodysize + kHeaderLen;
    }
}

void BinaryCodec::RecvMultiGetData(evpp::Buffer* buf, const std::size_t size, CommandPtr& cmd) {
    const char* data = buf->data();
    std::size_t len = 0;
    long extlength = 0;
    long bodylength = 0;
    long keylength = 0;
    uint16_t status = 0;
    std::string key;
    std::string value;
    std::map<std::string, std::string> result;
    const char* key_pos = data;
    while (len < size) {
        status = cmdstatus(data);
        if (status != PROTOCOL_BINARY_RESPONSE_SUCCESS && status != PROTOCOL_BINARY_RESPONSE_KEY_ENOENT) {
            cmd->error((EvmcCode)status);
            return;
        }
        bodylength = bodylen(data);
        keylength = keylen(data);
        extlength = extlen(data);
        key_pos = data + kHeaderLen + extlength;
        len += kHeaderLen + bodylength;
        data += kHeaderLen + bodylength;
        if (status == PROTOCOL_BINARY_RESPONSE_KEY_ENOENT) {
            continue;
        }
        key.assign(key_pos, keylength);
        value.assign(key_pos + keylength, bodylength - keylength - extlength);
        result.emplace(std::move(key), std::move(value));
    }
    auto ret = folly::makeExpected<EvmcCode>(std::move(result));
    static_cast<MultiGetCommand*>(cmd.get())->set_result(ret);
}

uint32_t BinaryCodec::OnRecvPrefixGetData(const char*   buf, EvmcCode& code
                                          , std::map<std::string, std::map<std::string, std::string>>& results) {
    const char*  data = buf;
    std::string key;
    std::string value;
    uint32_t klen = 0;
    uint32_t vlen = 0;
    std::size_t pos = 0;
    std::size_t  end = 0;

    uint16_t status = cmdstatus(data);
    uint16_t keylength = keylen(data);
    uint16_t extlength = extlen(data);
    uint32_t bodylength = bodylen(data);
    key.assign(data + kHeaderLen + extlength, keylength);
    if (status != PROTOCOL_BINARY_RESPONSE_SUCCESS || key.empty()) {
        code = (EvmcCode)(status);
        if (!key.empty() && status == PROTOCOL_BINARY_RESPONSE_KEY_ENOENT) {
            results.emplace(std::move(key), std::map<std::string, std::string>());
        }
        return bodylength + kHeaderLen;
    }
    pos = kHeaderLen + extlength + keylength;
    end = kHeaderLen + bodylength;
    while (pos < end) {
        if ((pos + 4) >= end) {
            break;
        }
        klen = ntohl((*(const uint32_t*)(data + pos)));
        pos += 4;
        if ((pos + klen) >= end) {
            break;
        }
        std::string rkey(data + pos, klen);
        pos += klen;
        if (pos + 4 >= end) {
            break;
        }
        vlen = ntohl(*(const uint32_t*)(data + pos));
        pos += 4;
        if ((pos + vlen) > end) {
            break;
        }
        std::string rval(data + pos, vlen);
        results[key].emplace(std::move(rkey), std::move(rval));
        pos += vlen;
    }
    code = EVMC_SUCCESS;
    return bodylength + kHeaderLen;
}

void BinaryCodec::RecvPrefixGetData(const evpp::Buffer* const buf, const std::size_t size, CommandPtr& cmd) {
    EvmcCode code = EVMC_SUCCESS;
    uint32_t len = 0;
    uint32_t cmdlen = 0;
    const char* data = buf->data();
    std::map<std::string, std::map<std::string, std::string>> results;
    while (len < size) {
        cmdlen = OnRecvPrefixGetData(data, code, results);
        if (code != EVMC_SUCCESS && code != EVMC_KEY_ENOENT) {
            static_cast<PrefixGetCommand*>(cmd.get())->error(code);
            return;
        }
        data = data + cmdlen;
        len += cmdlen;
    }
    assert(len == size);
    auto ret = folly::makeExpected<EvmcCode>(std::move(results));
    static_cast<PrefixGetCommand*>(cmd.get())->set_result(ret);
}

void BinaryCodec::RecvGetData(const void* data, CommandPtr& cmd) {
    uint16_t extlength = extlen(data);
    uint32_t bodylength = bodylen(data);
    uint16_t status = cmdstatus(data);
    const char* pv = (const char*)data + kHeaderLen + extlength;
    std::string value(pv, bodylength - extlength);
    GetCommand* command = static_cast<GetCommand*>(cmd.get());
    if (status == PROTOCOL_BINARY_RESPONSE_SUCCESS) {
        auto ret = folly::makeExpected<EvmcCode>(std::move(value));
        command->set_result(ret);
    } else {
        command->error((EvmcCode)status);
    }
}

void BinaryCodec::OnResponsePacket(evpp::Buffer* buf, const std::size_t size) {

    const void* data = buf->data();
    CommandPtr cmd = memc_client_->PeekRunningCommand();
    uint32_t id  = cmdid(data);  // no need to call ntohl
    int opcode = opcode(data);
    if (!cmd || id != cmd->recv_id()) {
        LOG_WARN << "OnResponsePacket cmd/message mismatch." << id << ", size=" << size << ",cmdid=" << (!cmd ? 0 : cmd->recv_id());
        return;
    }
    cmd = memc_client_->PopRunningCommand();
    switch (opcode) {
    case PROTOCOL_BINARY_CMD_SET: {
        uint16_t status = cmdstatus(data);
        auto ret = folly::makeExpected<EvmcCode>((int)status);
        static_cast<SetCommand*>(cmd.get())->set_result(ret);
    }
    break;

    case PROTOCOL_BINARY_CMD_DELETE: {
        uint16_t status = cmdstatus(data);
        auto ret = folly::makeExpected<EvmcCode>((int)status);
        static_cast<RemoveCommand*>(cmd.get())->set_result(ret);
    }
    break;

    case PROTOCOL_BINARY_CMD_GET:
        RecvGetData(data, cmd);
        break;

    case PROTOCOL_BINARY_CMD_GETK:
    case PROTOCOL_BINARY_CMD_GETKQ:
        RecvMultiGetData(buf, size, cmd);
        break;

    case PROTOCOL_BINARY_CMD_PGETK:
    case PROTOCOL_BINARY_CMD_PGETKQ:
        RecvPrefixGetData(buf, size, cmd);
        break;

    case PROTOCOL_BINARY_CMD_NOOP:
        LOG_DEBUG << "GETQ, NOOP opaque=" << id;
        //memc_client_->onMultiGetCommandDone(id, resp.response.status);
        break;
    default:
        break;
    }
}
}
