#include "memcache_client_single.h"
#include "evpp/buffer.h"
#include "evpp/tcp_conn.h"
#include "evpp/timestamp.h"

#include "vbucket_config.h"
#include "evpp/event_loop_thread_pool.h"
#include <memcached/protocol_binary.h>

namespace evmc {

const static int kHeaderLen = 24;
MemcacheClientSingle::~MemcacheClientSingle() {
	delete tcp_client_;
}

void MemcacheClientSingle::Stop(bool wait_thread_exit) {
}

CommandSinglePtr MemcacheClientSingle::PeekRunningCommand() {
	if (running_command_.empty()) {
		return CommandSinglePtr();
	} else {
		auto cmd = running_command_.front();
		return cmd;
	}
}

void MemcacheClientSingle::PushRunningCommand(CommandSinglePtr& cmd) {
	if (cmd) {
		running_command_.push(cmd);
	}
}

void MemcacheClientSingle::PopRunningCommand() {
	if (!running_command_.empty()) {
		running_command_.pop();
	}
}

void MemcacheClientSingle::PushWaitingCommand(CommandSinglePtr& cmd) {
        if (cmd) {
			waiting_command_.push(cmd);
        }
}

CommandSinglePtr MemcacheClientSingle::PeekPopWaitingCommand() {
		if (waiting_command_.empty()) {
			return CommandSinglePtr();
		}
		CommandSinglePtr command(waiting_command_.front());
		waiting_command_.pop();
		return command;
}

bool MemcacheClientSingle::Start(evpp::EventLoop * loop) {
    MultiModeVbucketConfigPtr vbconf = std::make_shared<MultiModeVbucketConfig>();
    bool success = vbconf->Load(vbucket_conf_file_.c_str());
    if (!success) {
        LOG_WARN << "DoReloadConf load err, file=" << vbucket_conf_file_;
        return false;
    }
	loop_ = loop;
	assert(NULL != loop_);
    auto server_list = vbconf->server_list();
	tcp_client_ = new evpp::TCPClient(loop_, server_list[0], "evmc");

	LOG_INFO << "Start new tcp_client=" << tcp_client_ << " server=" << server_list[0];

	tcp_client_->SetConnectionCallback(std::bind(&MemcacheClientSingle::OnClientConnection, this,
				std::placeholders::_1));
	tcp_client_->SetMessageCallback(std::bind(&MemcacheClientSingle::OnResponse, this,
				std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
	tcp_client_->Connect();
    return true;
}

void MemcacheClientSingle::OnClientConnection(const evpp::TCPConnPtr& conn) {
    LOG_INFO << "OnClientConnection conn=" << conn.get();

    if (conn && conn->IsConnected()) {
        LOG_INFO << "OnClientConnection connect ok";

		CommandSinglePtr command;
        while (command = PeekPopWaitingCommand()) {
        //    PushRunningCommand(command);
            SendData(command);
        }
    } else {
		if (conn) {
            LOG_INFO << "Disconnected from " << conn->remote_addr();
        } else {
            LOG_INFO << "Connect init error";
        }

		assert(loop_->IsInLoopThread());
		CommandSinglePtr command;
        while (command = PeekPopWaitingCommand()) {
                command->Error(ERR_CODE_NETWORK);
            }
        while (command = PeekRunningCommand()) {
                command->Error(ERR_CODE_NETWORK);
				PopRunningCommand();
            }
        }
}

void MemcacheClientSingle::MultiGet2(evpp::EventLoop* caller_loop, const std::vector<std::string>& keys, MultiGetCallback2 callback) {
    if (keys.size() <= 0) {
        return;
    }
	assert(loop_->IsInLoopThread());
	CommandSinglePtr cmd(new CommandSingle(caller_loop, callback, thread_id_++));
	Packet(keys, cmd->buf(), cmd->id());
	SendData(cmd);
}

void MemcacheClientSingle::OnResponse(const evpp::TCPConnPtr& conn,
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
		}
        if (buf->size() < resp.response.bodylen + kHeaderLen) {
			break;
		}

		uint32_t id  = resp.response.opaque;  // no need to call ntohl
		auto cmd =  PeekRunningCommand();
		assert(cmd != NULL);
		if (!cmd || cmd->id() != id) {
			buf->Retrieve(kHeaderLen + resp.response.bodylen);
			LOG_WARN << "OnResponsePacket cmd/message mismatch." << id;
			continue;
		}
		int opcode = resp.response.opcode;
		const int extlen = resp.response.extlen;
		const int keylen = resp.response.keylen; 
		const char* pv = buf->data() + sizeof(resp) + extlen;

		if (opcode == PROTOCOL_BINARY_CMD_GETK) {
			std::string key(pv, keylen);
			std::string value(pv + keylen, resp.response.bodylen - keylen - extlen);
			GetResult gt(resp.response.status, std::move(value));
			cmd->Collect(key, gt, true);
			PopRunningCommand();
		} else if (opcode == PROTOCOL_BINARY_CMD_GETKQ) {
			std::string key(pv, keylen);
			std::string value(pv + keylen, resp.response.bodylen - keylen - extlen);
			GetResult gt(resp.response.status, std::move(value));
			cmd->Collect(key, gt, false);
		} else {
		LOG_WARN << "unrecognize cmd opcode" << opcode;
		}
		buf->Retrieve(kHeaderLen + resp.response.bodylen);
	}
}

void MemcacheClientSingle::OnPacketTimeout(uint32_t cmd_id) {
    LOG_WARN << "InvokeTimer triggered for " << cmd_id;
	auto cmd = PeekRunningCommand();
    while (cmd) {
		cmd->Error(ERR_CODE_TIMEOUT);
		if (cmd->id() == cmd_id) {
			break;
        }
		PopRunningCommand();
		cmd = PeekRunningCommand();
    }
}


void MemcacheClientSingle::Packet(const std::vector<std::string> & keys, std::string& data, const uint32_t id) {

    for (size_t i = 0; i < keys.size(); ++i) {
        protocol_binary_request_header req;
        memset((void*)&req, 0, sizeof(req));
        req.request.magic = PROTOCOL_BINARY_REQ;

        if (i < keys.size() - 1) {
            req.request.opcode   = PROTOCOL_BINARY_CMD_GETKQ;
        } else {
            req.request.opcode   = PROTOCOL_BINARY_CMD_GETK;
        }

        req.request.keylen = htons(uint16_t(keys[i].size()));
        req.request.datatype = PROTOCOL_BINARY_RAW_BYTES;
        req.request.vbucket  = 0;
        req.request.opaque   = id;
        req.request.bodylen  = htonl(keys[i].size());
		data.append((char*)&req, sizeof(req));
		data.append(keys[i].data(), keys[i].size());
	}
}


void MemcacheClientSingle::SendData(CommandSinglePtr& command) {

    if (!tcp_client_->conn() || tcp_client_->conn()->status() == evpp::TCPConn::kConnecting) {
        PushWaitingCommand(command);
    } else if (tcp_client_->conn()->IsConnected()) {
        PushRunningCommand(command);
		tcp_client_->conn()->Send(command->buf().c_str(), command->buf().size());
		if (!timeout_.IsZero()) {
        if (cmd_timer_) {
            cmd_timer_->Cancel();
            //LOG_DEBUG << "InvokeTimer canceled for " << " " << conn()->remote_addr();
        }

        cmd_timer_= loop_->RunAfter(timeout_,
				std::bind(&MemcacheClientSingle::OnPacketTimeout, this, command->id()));

    }
    } else {
		assert(loop_->IsInLoopThread());
		command->Error(ERR_CODE_NETWORK);
	}
}
}

