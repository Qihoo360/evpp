#include "binary_codec.h"

#include "memcache_client.h"

namespace evmc {

static const size_t kHeaderLen = sizeof(protocol_binary_response_header);

void BinaryCodec::OnCodecMessage(const evpp::TCPConnPtr& conn,
       evpp::Buffer* buf,
       evpp::Timestamp ts) {
    while (buf->size() >= kHeaderLen) // kHeaderLen == 24
    {
        const void* data = buf->data();
        protocol_binary_response_header resp = 
            *static_cast<const protocol_binary_response_header*>(data);
        resp.response.bodylen = ntohl(resp.response.bodylen);
        resp.response.status  = ntohs(resp.response.status);
        resp.response.keylen  = ntohs(resp.response.keylen);
        if (resp.response.bodylen < 0) {
            LOG_ERROR << "Invalid length " << resp.response.bodylen;
            conn->Close();
            break;
        } else if (buf->size() >= resp.response.bodylen + kHeaderLen) {
            OnResponsePacket(resp, buf);
        } else {
            LOG_TRACE << "need recv more data";
            break;
        }
    }
}

void BinaryCodec::OnResponsePacket(const protocol_binary_response_header& resp,
            evpp::Buffer* buf) {
    uint32_t id  = resp.response.opaque;  // no need to call ntohl
    int      opcode = resp.response.opcode;
    CommandPtr cmd = memc_client_->peek_running_command();
    if (!cmd || id != cmd->id()) {
        // TODO : id 不一致时候，如何处理?
        buf->Retrieve(kHeaderLen + resp.response.bodylen);
        LOG_WARN << "OnResponsePacket cmd/message mismatch.";
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
        case PROTOCOL_BINARY_CMD_GET:
            {
                cmd = memc_client_->PopRunningCommand();
                const char* pv = buf->data() + sizeof(resp) + resp.response.extlen;
                std::string value(pv, resp.response.bodylen - resp.response.extlen);
                cmd->OnGetCommandDone(resp.response.status, value);
                LOG_DEBUG << "OnResponsePacket GET, opaque=" << id;
            }
            break;
        case PROTOCOL_BINARY_CMD_GETK:
            {
                cmd = memc_client_->PopRunningCommand();

                const char* pv = buf->data() + sizeof(resp) + resp.response.extlen;
                std::string key(pv, resp.response.keylen);
                std::string value(pv, resp.response.bodylen - resp.response.keylen - resp.response.extlen);

                cmd->OnMultiGetCommandDone(resp.response.status, key, value);
                LOG_DEBUG << "OnResponsePacket GETK, opaque=" << id << " key=" << key;
            }
            break;
        case PROTOCOL_BINARY_CMD_GETKQ:
            {
                cmd = memc_client_->peek_running_command();
                const char* pv = buf->data() + sizeof(resp) + resp.response.extlen;
                std::string key(pv, resp.response.keylen);
                std::string value(pv, resp.response.bodylen - resp.response.keylen - resp.response.extlen);

                cmd->OnMultiGetCommandOneResponse(resp.response.status, key, value);
                LOG_DEBUG << "OnResponsePacket GETKQ, opaque=" << id << " key=" << key;
            }
            break;
        case PROTOCOL_BINARY_CMD_NOOP:
          ////LOG_DEBUG << "GETQ, NOOP opaque=" << id;
          //memc_client_->onMultiGetCommandDone(id, resp.response.status);
          //break;
        default:
            break;
    }
    // memc_client_->set_last_command_id(id);
    buf->Retrieve(kHeaderLen + resp.response.bodylen);
}

}
