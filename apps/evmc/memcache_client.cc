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

    if (UNLIKELY(!timeout_.IsZero() && timer_canceled_)) {
        cmd_timer_ = exec_loop_->RunAfter(timeout_, std::bind(&MemcacheClient::OnPacketTimeout, shared_from_this(), cmd->id()));
        timer_canceled_ = false;
    }
}

void MemcacheClient::PushWaitingCommand(CommandPtr& cmd) {
    if (LIKELY(cmd)) {
        cmd->set_id(next_id());
        waiting_command_.push(cmd);
    }
    if (UNLIKELY(!timeout_.IsZero() && con_timer_canceled_)) {
        con_cmd_timer_ = exec_loop_->RunAfter(timeout_, std::bind(&MemcacheClient::OnConnectTimeout, shared_from_this(), cmd->id()));
        con_timer_canceled_ = false;
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
                                    evpp::Buffer* buf) {
    if (!codec_) {
        codec_ = new BinaryCodec(this);
    }

    codec_->OnCodecMessage(tcp_conn, buf);
}


void MemcacheClient::OnConnectTimeout(uint32_t cmd_id) {
    con_timer_canceled_ = true;
    if (!waiting_command_.empty()) {
        CommandPtr cmd(waiting_command_.front());
        if (LIKELY(cmd->id() != cmd_id)) {
            cmd_timer_bakup_.swap(con_cmd_timer_);
            con_cmd_timer_ = exec_loop_->RunAfter(timeout_, std::bind(&MemcacheClient::OnConnectTimeout, shared_from_this(), cmd->id()));
            con_timer_canceled_ = false;
            return;
        }
    } else {
        return;
    }

    LOG_DEBUG << "InvokeTimer triggered for " << cmd_id << " " << conn()->remote_addr();

    while (!waiting_command_.empty()) {
        CommandPtr cmd(waiting_command_.front());
        waiting_command_.pop();

        if (mc_pool_ && cmd->ShouldRetry()) {
            cmd->set_id(0);
            cmd->set_server_id(cmd->server_id());
            exec_loop()->RunInLoop(std::bind(&MemcacheClientBase::LaunchCommand, mc_pool_, cmd));
        } else {
            cmd->OnError(ERR_CODE_TIMEOUT);
        }
    }
}

void MemcacheClient::OnPacketTimeout(uint32_t cmd_id) {
    timer_canceled_ = true;
    if (!running_command_.empty()) {
        CommandPtr cmd(running_command_.front());
        if (LIKELY(cmd->id() != cmd_id)) {
            cmd_timer_bakup_.swap(cmd_timer_);
            cmd_timer_ = exec_loop_->RunAfter(timeout_, std::bind(&MemcacheClient::OnPacketTimeout, shared_from_this(), cmd->id()));
            timer_canceled_ = false;
            return;
        }
    } else {
        return;
    }

    LOG_DEBUG << "InvokeTimer triggered for " << cmd_id << " " << conn()->remote_addr();

    while (!running_command_.empty()) {
        CommandPtr cmd(running_command_.front());
        running_command_.pop();

        if (mc_pool_ && cmd->ShouldRetry()) {
            cmd->set_id(0);
            cmd->set_server_id(cmd->server_id());
            exec_loop()->RunInLoop(std::bind(&MemcacheClientBase::LaunchCommand, mc_pool_, cmd));
        } else {
            cmd->OnError(ERR_CODE_TIMEOUT);
        }

        if (cmd->id() == cmd_id) {
            break;
        }
    }
    LOG_ERROR << "OnPacketTimeout post, waiting=" << waiting_command_.size()
              << " running=" << running_command_.size();
}

}
