#include "binary_codec.h"

#include "memcache_client.h"

namespace evmc {

static const size_t kHeaderLen = sizeof(protocol_binary_response_header);

void BinaryCodec::OnCodecMessage(const evpp::TCPConnPtr& conn,
                                 evpp::Buffer* buf,
                                 evpp::Timestamp ts) {
    while (buf->size() >= kHeaderLen) { // kHeaderLen == 24
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


void BinaryCodec::DecodePrefixGetPacket(const protocol_binary_response_header& resp,
                                   evpp::Buffer* buf, CommandPtr& ptr) {
	const char* pv = buf->data() + sizeof(resp) + resp.response.extlen;
	std::string key(pv, resp.response.keylen);
	uint32_t pos = resp.response.keylen;
	uint32_t klen = 0;
	uint32_t vlen = 0;
	const uint32_t size = resp.response.bodylen - resp.response.keylen - resp.response.extlen; 
	const uint32_t buf_size = buf->size();
	int ret = resp.response.status;
	union {
		uint32_t len;
		char buf[4];
	} x;
	LOG_DEBUG << "Begin to parse key=" << key;
	while(pos < size) {
		if (pos >= buf_size || pos + 4 >= buf_size) {
			break;
		}
		//LOG_DEBUG << "parse inner begin key=" << key;
		//memcpy(x.buf, pv + pos, 4);
		klen = ntohl((*(uint32_t *)(pv + pos)));
		pos += 4;
		//klen = ntohl(x.len);
		if ((pos + klen) >= buf_size) {
			break;
		}
		std::string rkey(pv + pos, klen);
		pos += klen;                                                                                                                                                                                                                                                      
		if (pos >= buf_size || pos + 4 >= buf_size) {
			break;
		}
		//memcpy(x.buf, pv + pos, 4);
		vlen = ntohl(*(uint32_t *)(pv + pos));
		pos += 4;
		//vlen = ntohl(x.len);
		if ((pos + vlen) >= buf_size) {
			break;
		}
		std::string rval(pv + pos, vlen);
		//LOG_DEBUG << "key=" << key << ":" << size << " parse prefix:" << rkey << " get result, GetResult value: " << rval;
		ptr->OnPrefixGetCommandOneResponse(rkey, rval);
		//LOG_DEBUG << "parse inner end key=" << key;
		pos += vlen;
	}
	ptr->OnPrefixGetCommandDone(ret, key);
}

void BinaryCodec::OnResponsePacket(const protocol_binary_response_header& resp,
                                   evpp::Buffer* buf) {
    uint32_t id  = resp.response.opaque;  // no need to call ntohl
    int opcode = resp.response.opcode;
    CommandPtr cmd = memc_client_->peek_running_command();
    if (!cmd || id != cmd->id()) {
        // TODO : id 不一致时候，如何处理?
#if 0
        const char* pv = buf->data() + sizeof(resp) + resp.response.extlen;
        std::string value(pv, resp.response.bodylen - resp.response.extlen);
		LOG_WARN << "unexpected packet info:" << value;
#endif
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

        const char* pv = buf->data() + sizeof(resp) + resp.response.extlen;
        std::string key(pv, resp.response.keylen);
        std::string value(pv + resp.response.keylen, resp.response.bodylen - resp.response.keylen - resp.response.extlen);

        cmd->OnMultiGetCommandDone(resp.response.status, key, value);
        LOG_DEBUG << "OnResponsePacket GETK, opaque=" << id << " key=" << key << " value=" << value << " retcode=" << resp.response.status;
    }
    break;

    case PROTOCOL_BINARY_CMD_GETKQ: {
        cmd = memc_client_->peek_running_command();
        const char* pv = buf->data() + sizeof(resp) + resp.response.extlen;
        std::string key(pv, resp.response.keylen);
        std::string value(pv + resp.response.keylen, resp.response.bodylen - resp.response.keylen - resp.response.extlen);

        cmd->OnMultiGetCommandOneResponse(resp.response.status, key, value);
        LOG_DEBUG << "OnResponsePacket GETKQ, opaque=" << id << " key=" << key;
    }
    break;

	case PROTOCOL_BINARY_CMD_PGETK: 
        cmd = memc_client_->PopRunningCommand();
		DecodePrefixGetPacket(resp, buf, cmd);
        LOG_DEBUG << "OnResponsePacket PGETK";
		break;

	case PROTOCOL_BINARY_CMD_PGETKQ: 
        cmd = memc_client_->peek_running_command();
		DecodePrefixGetPacket(resp, buf, cmd);
        LOG_DEBUG << "OnResponsePacket PGETKQ";
		if (cmd->IsDone()) {
			memc_client_->PopRunningCommand();
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
