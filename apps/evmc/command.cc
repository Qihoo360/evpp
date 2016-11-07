#include "command.h"

#include <memcached/protocol_binary.h>

#include "memcache_client.h"
#include "vbucket_config.h"

namespace evmc {

void Command::Launch(MemcacheClientPtr memc_client) {
    if (!memc_client->conn()) {
        // TODO : 触发网络错误回调
        LOG_ERROR << "Command bad memc_client " << memc_client << " id=" << id_;
        return;
    }
	auto buf = RequestBuffer(); 
    memc_client->conn()->Send(buf->data(), buf->size());
}

uint16_t Command::server_id() const {
    if (server_id_history_.empty()) {
        return BAD_SERVER_ID;
    }

    return server_id_history_.back();
}

bool Command::ShouldRetry() const {
    LOG_DEBUG << "ShouldRetry vbucket=" << vbucket_id()
              << " server_id=" << server_id()
              << " len=" << server_id_history_.size();
    return server_id_history_.size() < 2;
}

std::atomic_int SetCommand::next_thread_;
BufferPtr SetCommand::RequestBuffer()  {
    protocol_binary_request_header req;
    memset((void*)&req, 0, sizeof(req));

    req.request.magic  = PROTOCOL_BINARY_REQ;
    req.request.opcode = PROTOCOL_BINARY_CMD_SET;
    req.request.keylen = htons(uint16_t(key_.size()));
    req.request.extlen = 8;
    req.request.datatype = PROTOCOL_BINARY_RAW_BYTES;
    req.request.vbucket  = htons(vbucket_id());
    req.request.opaque   = id();
    size_t bodylen = req.request.extlen + key_.size() + value_.size();
    req.request.bodylen = htonl(static_cast<uint32_t>(bodylen));

    BufferPtr buf(new evpp::Buffer(sizeof(protocol_binary_request_header) + key_.size()));
    buf->Append((void*)&req, sizeof(req));
    buf->AppendInt32(flags_);
    buf->AppendInt32(expire_);
    buf->Append(key_.data(), key_.size());
    buf->Append(value_.data(), value_.size());

    return buf;
}

std::atomic_int PrefixGetCommand::next_thread_;
BufferPtr PrefixGetCommand::RequestBuffer()  {
    protocol_binary_request_header req;
    memset((void*)&req, 0, sizeof(req));

    req.request.magic  = PROTOCOL_BINARY_REQ;
    req.request.opcode = PROTOCOL_BINARY_CMD_PGETK; 
    req.request.keylen = htons(uint16_t(key_.size()));
    req.request.datatype = PROTOCOL_BINARY_RAW_BYTES;
    req.request.vbucket  = htons(vbucket_id());
    req.request.opaque   = id();
    req.request.bodylen = htonl(key_.size());

    BufferPtr buf(new evpp::Buffer(sizeof(protocol_binary_request_header) + key_.size()));
    buf->Append((void*)&req, sizeof(req));
    buf->Append(key_.data(), key_.size());
    return buf;
}

void PrefixGetCommand::OnPrefixGetCommandDone() {
    if (caller_loop()) {
        caller_loop()->RunInLoop(std::bind(mget_callback_, std::move(key_),  mget_result_));
    } else {
        mget_callback_(key_, mget_result_);
    }
}

std::atomic_int GetCommand::next_thread_;
BufferPtr GetCommand::RequestBuffer()  {
    protocol_binary_request_header req;
    memset((void*)&req, 0, sizeof(req));

    req.request.magic  = PROTOCOL_BINARY_REQ;
	req.request.opcode = PROTOCOL_BINARY_CMD_GET;
    req.request.keylen = htons(uint16_t(key_.size()));
    req.request.datatype = PROTOCOL_BINARY_RAW_BYTES;
    req.request.vbucket  = htons(vbucket_id());
    req.request.opaque   = id();
    req.request.bodylen = htonl(key_.size());

    BufferPtr buf(new evpp::Buffer(sizeof(protocol_binary_request_header) + key_.size()));
    buf->Append((void*)&req, sizeof(req));
    buf->Append(key_.data(), key_.size());

    return buf;
}

void MultiGetCommand::OnMultiGetCommandDone(int resp_code, std::string& key, std::string& value) {
	OnMultiGetCommandOneResponse(resp_code, key, value);
    get_handler()->get_result().code = resp_code;
	const uint32_t finish = get_handler()->FinishedOne();
	if ((finish + 1) >= get_handler()->vbucket_size()) {
		caller_loop()->RunInLoop(std::bind(get_handler()->get_callback(), std::move(get_handler()->get_result())));
	}
}

void MultiGetCommand::OnMultiGetCommandOneResponse(int resp_code, std::string& key, std::string& value) {
	auto & result_map = get_handler()->get_result().get_result_map_;
	auto  it = result_map.find(key);
	assert(it != result_map.end());
	auto & get_result = it->second;
    if (resp_code == PROTOCOL_BINARY_RESPONSE_SUCCESS) {
		get_result.code = resp_code;
		get_result.value.swap(value);
    } else {
		get_result.code = resp_code;
	}
}

BufferPtr MultiGetCommand::RequestBuffer()  {
	auto& keys = get_handler()->FindKeysByid(vbucket_id());
	std::size_t keys_num = keys.size();
    BufferPtr buf(new evpp::Buffer(50 * keys_num)); // 预分配长度大多数时候是足够的

	std::size_t size = 0;
	protocol_binary_request_header req;
    for (size_t i = 0; i < keys_num; ++i) {
		size = keys[i].size();
        memset((void*)&req, 0, sizeof(req));
        req.request.magic    = PROTOCOL_BINARY_REQ;

        if (i < keys_num - 1) {
            req.request.opcode   = PROTOCOL_BINARY_CMD_GETKQ;
        } else {
            req.request.opcode   = PROTOCOL_BINARY_CMD_GETK;
        }

        req.request.keylen = htons(uint16_t(size));
        req.request.datatype = PROTOCOL_BINARY_RAW_BYTES;
        req.request.vbucket  = htons(vbucket_id());
        req.request.opaque   = id();
        req.request.bodylen  = htonl(size);

        buf->Append((void*)&req, sizeof(req));
        buf->Append(keys[i].data(), size);
    }
    return buf;
}

BufferPtr PrefixMultiGetCommand::RequestBuffer()  {
	auto& keys = get_handler()->FindKeysByid(vbucket_id());
	std::size_t keys_num = keys.size();
    BufferPtr buf(new evpp::Buffer(50 * keys_num)); // 预分配长度大多数时候是足够的

	std::size_t size = 0;
	protocol_binary_request_header req;
    for (size_t i = 0; i < keys_num; ++i) {
		size = keys[i].size();
        memset((void*)&req, 0, sizeof(req));
        req.request.magic    = PROTOCOL_BINARY_REQ;

        if (i < keys_num - 1) {
			req.request.opcode   = PROTOCOL_BINARY_CMD_PGETKQ;  
        } else {
            req.request.opcode   = PROTOCOL_BINARY_CMD_PGETK;
        }

        req.request.keylen = htons(uint16_t(size));
        req.request.datatype = PROTOCOL_BINARY_RAW_BYTES;
        req.request.vbucket  = htons(vbucket_id());
        req.request.opaque   = id();
        req.request.bodylen  = htonl(size);

        buf->Append((void*)&req, sizeof(req));
        buf->Append(keys[i].data(), size);
    }
    return buf;
}

void PrefixMultiGetCommand::OnPrefixGetCommandDone() {
	const uint32_t finish = get_handler()->FinishedOne();
	if ((finish + 1) >= get_handler()->vbucket_size()) {
		get_handler()->get_result().code = SUC_CODE;  
		caller_loop()->RunInLoop(std::bind(get_handler()->get_callback(), std::move(get_handler()->get_result())));
	}
}
	
PrefixGetResultPtr& PrefixMultiGetCommand::GetResultContainerByKey(const std::string& key) {
	auto & result_map = get_handler()->get_result().get_result_map_;
	auto  it = result_map.find(key);
	assert(it != result_map.end());
	return it->second;
}


std::atomic_int RemoveCommand::next_thread_;
BufferPtr RemoveCommand::RequestBuffer()  {
    protocol_binary_request_header req;
    memset((void*)&req, 0, sizeof(req));

    req.request.magic  = PROTOCOL_BINARY_REQ;
    req.request.opcode = PROTOCOL_BINARY_CMD_DELETE;
    req.request.keylen = htons(uint16_t(key_.size()));
    req.request.datatype = PROTOCOL_BINARY_RAW_BYTES;
    req.request.vbucket  = htons(vbucket_id());
    req.request.opaque   = id();
    req.request.bodylen = htonl(static_cast<uint32_t>(key_.size()));

    BufferPtr buf(new evpp::Buffer(sizeof(protocol_binary_request_header) + key_.size()));
    buf->Append((void*)&req, sizeof(req));
    buf->Append(key_.data(), key_.size());

    return buf;
}

}


