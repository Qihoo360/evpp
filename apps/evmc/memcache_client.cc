#include "memcache_client.h"

#include "binary_codec.h"
#include "memcache_client_pool.h"
#include "likely.h"

namespace evmc {

MemcacheClient::~MemcacheClient() {
    tcp_client_->Disconnect();
    delete codec_;
}

void MemcacheClient::PushRunningCommand(CommandPtr& cmd) {
    if (UNLIKELY(!cmd)) {
        return;
    }

    if (cmd->id() == 0) {
        cmd->set_id(next_id());
    }
    running_command_.emplace(cmd);

    if (UNLIKELY(!timeout_.IsZero() && !cmd_timer_)) {
			LOG_DEBUG << "Create Timer for " << " " << conn()->remote_addr();
			cmd_timer_ = exec_loop_->RunAfter(timeout_, std::bind(&MemcacheClient::OnPacketTimeout, shared_from_this(), cmd->id()));
	}
}

CommandPtr MemcacheClient::PopRunningCommand() {
    if (UNLIKELY(running_command_.empty())) {
        return CommandPtr();
    }

    CommandPtr command(running_command_.front());
    running_command_.pop();
    return command;
}

CommandPtr MemcacheClient::PopWaitingCommand() {
    if (UNLIKELY(waiting_command_.empty())) {
        return CommandPtr();
    }
    CommandPtr command(waiting_command_.front());
    waiting_command_.pop();
    return command;
}

void MemcacheClient::OnResponseData(const evpp::TCPConnPtr& tcp_conn,
                                    evpp::Buffer* buf,
                                    evpp::Timestamp ts) {
	if (!codec_) {
			codec_ = new BinaryCodec(this);
	}

	codec_->OnCodecMessage(tcp_conn, buf, ts);
}

void MemcacheClient::OnPacketTimeout(uint32_t cmd_id) {
	if (!running_command_.empty()) {
        CommandPtr cmd(running_command_.front());
		if (LIKELY(cmd->id() != cmd_id)) {
			cmd_timer_->Cancel();
			cmd_timer_ = exec_loop_->RunAfter(timeout_, std::bind(&MemcacheClient::OnPacketTimeout, shared_from_this(), cmd->id()));
			LOG_DEBUG << "Delete Timer for " << " " << conn()->remote_addr();
			return;
		}
	} else {
		LOG_DEBUG << "Delete Timer for " << " " << conn()->remote_addr();
		cmd_timer_->Cancel();
		cmd_timer_.reset();
		return;
	}

    LOG_DEBUG << "InvokeTimer triggered for " << cmd_id << " " << conn()->remote_addr();

    while (!running_command_.empty()) {
        CommandPtr cmd(running_command_.front());
        running_command_.pop();

        if (mc_pool_ && cmd->ShouldRetry()) {
            cmd->set_id(0);
            cmd->caller_loop()->RunInLoop(std::bind(&MemcacheClientBase::LaunchCommand, mc_pool_, cmd));
        } else {
            cmd->OnError(ERR_CODE_TIMEOUT);
        }

        if (cmd->id() == cmd_id) {
            break;
        }
    }
    LOG_WARN << "OnPacketTimeout post, waiting=" << waiting_command_.size()
             << " running=" << running_command_.size();
}

}
