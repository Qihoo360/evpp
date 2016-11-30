#include "command.h"
#include <memcached/protocol_binary.h>
#include "memcache_client.h"
#include "vbucket_config.h"
#include "likely.h"

namespace evmc {

void Command::Launch(MemcacheClientPtr memc_client) {
	std::string buf;
	RequestBuffer(buf); 
    memc_client->conn()->Send(buf.data(), buf.size());
}

uint16_t Command::server_id() const {
    if (UNLIKELY(server_id_history_.empty())) {
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
void SetCommand::RequestBuffer(std::string& buf)  {
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

    buf.resize(sizeof(protocol_binary_request_header) + key_.size());
	buf.clear();
    buf.append((const char *)&req, sizeof(req));
	auto flag = htonl(flags_);
    buf.append((const char *)&flag, sizeof(flag));
	auto expire = htonl(expire_);
    buf.append((const char *)&expire, sizeof(expire));
    buf.append(key_.data(), key_.size());
    buf.append(value_.data(), value_.size());
}

std::atomic_int PrefixGetCommand::next_thread_;
void PrefixGetCommand::RequestBuffer(std::string& buf)  {
    protocol_binary_request_header req;
    memset((void*)&req, 0, sizeof(req));

    req.request.magic  = PROTOCOL_BINARY_REQ;
    req.request.opcode = PROTOCOL_BINARY_CMD_PGETK; 
    req.request.keylen = htons(uint16_t(key_.size()));
    req.request.datatype = PROTOCOL_BINARY_RAW_BYTES;
    req.request.vbucket  = htons(vbucket_id());
    req.request.opaque   = id();
    req.request.bodylen = htonl(key_.size());

    buf.resize(sizeof(protocol_binary_request_header) + key_.size());
	buf.clear();
    buf.append((const char *)&req, sizeof(req));
    buf.append(key_.data(), key_.size());
}

void PrefixGetCommand::OnPrefixGetCommandDone() {
	auto loop = caller_loop(); 
	if (loop && !loop->IsInLoopThread()) {
        loop->RunInLoop(std::bind(mget_callback_, std::move(key_),  mget_result_));
    } else {
        mget_callback_(key_, mget_result_);
    }
}

std::atomic_int GetCommand::next_thread_;
void GetCommand::RequestBuffer(std::string& buf)  {
    protocol_binary_request_header req;
    memset((void*)&req, 0, sizeof(req));

    req.request.magic  = PROTOCOL_BINARY_REQ;
	req.request.opcode = PROTOCOL_BINARY_CMD_GET;
    req.request.keylen = htons(uint16_t(key_.size()));
    req.request.datatype = PROTOCOL_BINARY_RAW_BYTES;
    req.request.vbucket  = htons(vbucket_id());
    req.request.opaque   = id();
    req.request.bodylen = htonl(key_.size());

    buf.resize(sizeof(protocol_binary_request_header) + key_.size());
	buf.clear();
    buf.append((const char *)&req, sizeof(req));
    buf.append(key_.data(), key_.size());
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
	get_result.code = resp_code;
	get_result.value.swap(value);
}

void MultiGetCommand::PacketRequest(const std::vector<std::string>& keys, const uint16_t vbucket_id, const uint32_t id, std::string & buf) {
	std::size_t size = 0;
	protocol_binary_request_header req;
	const std::size_t keys_num = keys.size();
	buf.resize(65 * keys_num);
	buf.clear();
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
        req.request.vbucket  = htons(vbucket_id);
        req.request.opaque   = id;
        req.request.bodylen  = htonl(size);

        buf.append((const char *)&req, sizeof(req));
        buf.append(keys[i].data(), size);
    }
}

void MultiGetCommand::RequestBuffer(std::string& buf)  {
	auto& keys = get_handler()->FindKeysByid(vbucket_id());
	MultiGetCommand::PacketRequest(keys, vbucket_id(), id(), buf);
}

void SerialMultiGetCommand::OnMultiGetCommandDone(int resp_code, std::string& key, std::string& value) {
	SerialMultiGetCommand::OnMultiGetCommandOneResponse(resp_code, key, value);
	multiget_result_.code = SUC_CODE;
	callback_(std::move(multiget_result_));
}

void SerialMultiGetCommand::OnMultiGetCommandOneResponse(int resp_code, std::string& key, std::string& value) {
	GetResult get_result;
	get_result.code = resp_code;
	get_result.value.swap(value);
	multiget_result_.get_result_map_.emplace(std::move(key), std::move(get_result));
}

void SerialMultiGetCommand::Launch(MemcacheClientPtr memc_client) {
    memc_client->conn()->Send(buf_.data(), buf_.size());
}

void PrefixMultiGetCommand::PacketRequest(const std::vector<std::string>& keys, const uint16_t vbucket_id, const uint32_t id, std::string&  buf) {
	std::size_t size = 0;
	protocol_binary_request_header req;
	const std::size_t keys_num = keys.size();
	buf.resize(65 * keys_num);
	buf.clear();
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
        req.request.vbucket  = htons(vbucket_id);
        req.request.opaque   = id;
        req.request.bodylen  = htonl(size);

        buf.append((const char *)&req, sizeof(req));
        buf.append(keys[i].data(), size);
    }
}

void PrefixMultiGetCommand::RequestBuffer(std::string& buf)  {
	auto& keys = get_handler()->FindKeysByid(vbucket_id());
	PacketRequest(keys, vbucket_id(), id(), buf);
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
void RemoveCommand::RequestBuffer(std::string& buf)  {
    protocol_binary_request_header req;
    memset((void*)&req, 0, sizeof(req));

    req.request.magic  = PROTOCOL_BINARY_REQ;
    req.request.opcode = PROTOCOL_BINARY_CMD_DELETE;
    req.request.keylen = htons(uint16_t(key_.size()));
    req.request.datatype = PROTOCOL_BINARY_RAW_BYTES;
    req.request.vbucket  = htons(vbucket_id());
    req.request.opaque   = id();
    req.request.bodylen = htonl(static_cast<uint32_t>(key_.size()));

    buf.resize(sizeof(protocol_binary_request_header) + key_.size());
	buf.clear();
    buf.append((const char *)&req, sizeof(req));
    buf.append(key_.data(), key_.size());
}

}


