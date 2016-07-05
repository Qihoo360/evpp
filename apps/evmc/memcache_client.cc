#include "memcache_client.h"

#include "binary_codec.h"
#include "memcache_client_pool.h"

namespace evmc {

MemcacheClient::~MemcacheClient() {
    tcp_client_->Disconnect();
    delete codec_;
}

void MemcacheClient::PushRunningCommand(CommandPtr cmd) {
    exec_loop_->AssertInLoopThread();
    if (cmd->id() == 0) {
        cmd->set_id(next_id());
    }

    if (!cmd) {
        return;
    }
    running_command_.push(cmd);

    if (!timeout_.IsZero()) {
        if (cmd_timer_) {
            cmd_timer_->Cancel();
            LOG_DEBUG << "InvokeTimer canceled for " << " " << conn()->remote_addr();
        }

        evpp::InvokeTimerPtr timer = exec_loop_->RunAfter(timeout_, 
                std::bind(&MemcacheClient::OnPacketTimeout, shared_from_this(), cmd->id()));

        cmd_timer_ = timer;
    }
}

CommandPtr MemcacheClient::PopRunningCommand() {
    exec_loop_->AssertInLoopThread();

    if (running_command_.empty()) {
        return CommandPtr(); 
    }
    CommandPtr command(running_command_.front());
    running_command_.pop();

    if (!timeout_.IsZero() && running_command_.empty()) {
        if (cmd_timer_) {
            cmd_timer_->Cancel();
            cmd_timer_.reset();
        }
    }

    return command;
}

CommandPtr MemcacheClient::pop_waiting_command() {
    if (waiting_command_.empty()) {
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
    LOG_DEBUG << "InvokeTimer triggered for " << cmd_id << " " << conn()->remote_addr();
    // cmd_timer_->Cancel();
    // cmd_timer_.reset();
    while(!running_command_.empty()) {
        CommandPtr cmd(running_command_.front());
        running_command_.pop();

        if (mc_pool_ && cmd->ShouldRetry()) {
            // mc_pool_->LaunchCommand(cmd); 
            cmd->set_id(0); 
            cmd->caller_loop()->RunInLoop(std::bind(&MemcacheClientPool::LaunchCommand, mc_pool_, cmd));
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
