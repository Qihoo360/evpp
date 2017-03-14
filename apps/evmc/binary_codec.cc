#include "binary_codec.h"

#include "memcache_client.h"

namespace evmc {

static const size_t kHeaderLen = sizeof(protocol_binary_response_header);

void BinaryCodec::OnCodecMessage(const evpp::TCPConnPtr& conn,
                                 evpp::Buffer* buf) {
    while (buf->size() >= kHeaderLen) { // kHeaderLen == 24
        const void* data = buf->data();
        protocol_binary_response_header resp =
            *static_cast<const protocol_binary_response_header*>(data);
        resp.response.bodylen = ntohl(resp.response.bodylen);
        resp.response.status  = ntohs(resp.response.status);
        resp.response.keylen  = ntohs(resp.response.keylen);

        if (buf->size() >= resp.response.bodylen + kHeaderLen) {
            OnResponsePacket(resp, buf);
        } else {
            LOG_TRACE << "need recv more data";
            break;
        }
    }
}


void BinaryCodec::DecodePrefixGetPacket(const protocol_binary_response_header& resp,
                                        evpp::Buffer* buf, PrefixGetResultPtr& ptr) {
    const char* pv = buf->data() + sizeof(resp) + resp.response.extlen;
    uint32_t pos = resp.response.keylen;
    uint32_t klen = 0;
    uint32_t vlen = 0;
    const uint32_t size = resp.response.bodylen - resp.response.keylen - resp.response.extlen;
    const uint32_t buf_size = buf->size();
    int ret = resp.response.status;
    ptr->code = ret;
    while (pos < size) {
        if (pos >= buf_size || pos + 4 >= buf_size) {
            break;
        }
        klen = ntohl((*(const uint32_t*)(pv + pos)));
        pos += 4;
        if ((pos + klen) >= buf_size) {
            break;
        }
        std::string rkey(pv + pos, klen);
        pos += klen;
        if (pos >= buf_size || pos + 4 >= buf_size) {
            break;
        }
        vlen = ntohl(*(const uint32_t*)(pv + pos));
        pos += 4;
        if ((pos + vlen) >= buf_size) {
            break;
        }

        std::string rval(pv + pos, vlen);
        ptr->result_map_.emplace(rkey, rval);
        pos += vlen;
    }
}

void BinaryCodec::OnResponsePacket(const protocol_binary_response_header& resp,
                                   evpp::Buffer* buf) {
    uint32_t id  = resp.response.opaque;  // no need to call ntohl
    int opcode = resp.response.opcode;
    CommandPtr cmd = memc_client_->PeekRunningCommand();
    if (!cmd || id != cmd->id()) {
        // TODO : id 不一致时候，如何处理?
        buf->Retrieve(kHeaderLen + resp.response.bodylen);
        LOG_WARN << "OnResponsePacket cmd/message mismatch." << id;
        return;
    }

    switch (opcode) {
    case PROTOCOL_BINARY_CMD_SET:
        cmd = memc_client_->PopRunningCommand();
        cmd->OnSetCommandDone(resp.response.status);
        LOG_DEBUG << "OnResponsePacket SET, opaque=" << id;
        break;

    case PROTOCOL_BINARY_CMD_DELETE:
        cmd = memc_client_->PopRunningCommand();
        cmd->OnRemoveCommandDone(resp.response.status);
        LOG_DEBUG << "OnResponsePacket DELETE, opaque=" << id;
        break;

    case PROTOCOL_BINARY_CMD_GET: {
        cmd = memc_client_->PopRunningCommand();
        const char* pv = buf->data() + sizeof(resp) + resp.response.extlen;
        std::string value(pv, resp.response.bodylen - resp.response.extlen);
        cmd->OnGetCommandDone(resp.response.status, value);
        LOG_DEBUG << "OnResponsePacket GET, opaque=" << id;
    }
    break;

    case PROTOCOL_BINARY_CMD_GETK: {
        cmd = memc_client_->PopRunningCommand();
        const int extlen_getk = resp.response.extlen;
        const int keylen_getk = resp.response.keylen;
        const char* pv = buf->data() + sizeof(resp) + extlen_getk;
        std::string key(pv, keylen_getk);
        std::string value(pv + keylen_getk, resp.response.bodylen - keylen_getk - extlen_getk);

        cmd->OnMultiGetCommandDone(resp.response.status, key, value);
        LOG_DEBUG << "OnResponsePacket MULTIGETK, opaque=" << id;
    }
    break;

    case PROTOCOL_BINARY_CMD_GETKQ: {
        const int extlen = resp.response.extlen;
        const int keylen = resp.response.keylen;
        const char* pv = buf->data() + sizeof(resp) + extlen;
        std::string key(pv, keylen);
        std::string value(pv + keylen, resp.response.bodylen - keylen - extlen);

        cmd->OnMultiGetCommandOneResponse(resp.response.status, key, value);
        LOG_DEBUG << "OnResponsePacket MULTIGETQ, opaque=" << id;
    }
    break;

    case PROTOCOL_BINARY_CMD_PGETK: {
        cmd = memc_client_->PopRunningCommand();
        const char* pv = buf->data() + sizeof(resp) + resp.response.extlen;
        std::string key(pv, resp.response.keylen);
        if (!key.empty()) {
            auto result_ptr = cmd->GetResultContainerByKey(key);
            DecodePrefixGetPacket(resp, buf, result_ptr);
            cmd->OnPrefixGetCommandDone();
            LOG_DEBUG << "OnResponsePacket PGETK, opaque=" << id;
        } else {
            cmd->OnError(resp.response.status);
        }
    }
    break;

    case PROTOCOL_BINARY_CMD_PGETKQ: {
        const char* pv = buf->data() + sizeof(resp) + resp.response.extlen;
        std::string key(pv, resp.response.keylen);
        if (!key.empty()) {
            auto result_ptr = cmd->GetResultContainerByKey(key);
            DecodePrefixGetPacket(resp, buf, result_ptr);
            LOG_DEBUG << "OnResponsePacket PGETKQ" << id;
        } else {
            cmd->OnError(resp.response.status);
        }
    }
    break;

    case PROTOCOL_BINARY_CMD_NOOP:
        LOG_DEBUG << "GETQ, NOOP opaque=" << id;
        //memc_client_->onMultiGetCommandDone(id, resp.response.status);
        break;
    default:
        break;
    }
    buf->Retrieve(kHeaderLen + resp.response.bodylen);
}

}
