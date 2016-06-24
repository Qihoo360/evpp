#include "memcache_client.h"

#include "binary_codec.h"

namespace evmc {

MemcacheClient::~MemcacheClient() {
    tcp_client_->Disconnect();
    delete codec_;
}

void MemcacheClient::PushRunningCommand(CommandPtr cmd) {
    if (!cmd) {
        return;
    }
    running_command_.push(cmd);

    if (!timeout_.IsZero()) {
        // TODO: 权衡 caller_loop_ 是否要用 shared_ptr
        TimerEventPtr timer(new evpp::TimerEventWatcher(exec_loop_.get(),
                std::bind(&MemcacheClient::OnPacketTimeout, shared_from_this(), cmd->id()), timeout_));
        timer->Init();
        timer->AsyncWait();
        if (cmd_timer_) {
            cmd_timer_->Cancel();
        }
        cmd_timer_ = timer;
    }
}

CommandPtr MemcacheClient::PopRunningCommand() {
    if (running_command_.empty()) {
        return CommandPtr(); 
    }
    CommandPtr command(running_command_.front());
    running_command_.pop();

    if (!timeout_.IsZero() && running_command_.empty()) {
        cmd_timer_->Cancel();
        cmd_timer_.reset();
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
    LOG_WARN << "OnPacketTimeout cmd_id=" << cmd_id;
    while(!running_command_.empty()) {
        CommandPtr cmd(running_command_.front());
        running_command_.pop();

        LOG_WARN << "OnPacketTimeout cmd=" << cmd->id();
        cmd->OnError(ERR_CODE_TIMEOUT);
        if (cmd->id() == cmd_id) { // 不比大小比相等, 以绕过uint溢出的问题
            break;
        }
    }
}

}
