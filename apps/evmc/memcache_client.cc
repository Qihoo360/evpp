#include "memcache_client.h"

#include "binary_codec.h"
#include "memcache_client_pool.h"
#include "likely.h"
#include "evpp/evpp_wheeltimer_thread.h"

namespace evmc {

MemcacheClient::~MemcacheClient() {
    tcp_client_->Disconnect();
}

//called by wheeltimer thread
void MemcacheClient::OnTimeout(const uint32_t r_id) {
    int64_t recv_id_int64 = (int64_t)r_id;
    if (recv_id_int64 < recv_checkpoint_) {
        return;
    }
    recv_checkpoint_ = recv_id();
    exec_loop_->RunInLoop([this, r_id]() {
        this->OnPacketTimeout(r_id);
    });
}

void MemcacheClient::Launch(CommandPtr& cmd) {
    cmd->set_recv_id(next_send_id());
    cmd->construct_command(&send_buf_);
    running_command_.push(cmd);
    evpp::EvppHHWheelTimer::Instance()->scheduleTimeout([this, cmd_id = cmd->recv_id()]() {
        this->OnTimeout(cmd_id);
    }
    , timeout_);
    if (tcp_client_->conn() && tcp_client_->conn()->IsConnected()) {
        SendData();
    }
}

void MemcacheClient::SendData() {
    tcp_client_->conn()->Send(&send_buf_);
}

CommandPtr MemcacheClient::PopRunningCommand() {
    if (UNLIKELY(running_command_.empty())) {
        return CommandPtr();
    }

    CommandPtr command(running_command_.front());
    running_command_.pop();
    return command;
}

void MemcacheClient::OnClientConnection(const evpp::TCPConnPtr& connection) {
    if (connection && connection->IsConnected()) {
        //发送数据
        SendData();
        return;
    }
    CommandPtr command;
    if (connection) {
        LOG_WARN << "connect " << (int)(connection->status()) << "[0:kDisconnected,1:kConnecting,3:kDisconnecting],ip=" << connection->remote_addr() << ",pop command size=" << running_command_.size();
    }
    while (running_command_.size() > 0) {
        command = running_command_.front();
        running_command_.pop();
        mc_pool_->ReLaunchCommand(exec_loop_, command);
    }
    send_buf_.Reset();
}

void MemcacheClient::OnResponseData(const evpp::TCPConnPtr& tcp_conn,
                                    evpp::Buffer* buf) {
    if (!codec_) {
        codec_ = new BinaryCodec(this);
    }
    codec_->OnCodecMessage(tcp_conn, buf);
}

void MemcacheClient::OnPacketTimeout(const uint32_t cmd_id) {
    if (running_command_.size() == 0) {
        return;
    }
    auto& cmd = running_command_.front();
    if (cmd->recv_id() != cmd_id) {
        return;
    }
    CommandPtr command(running_command_.front());
    running_command_.pop();
    mc_pool_->ReLaunchCommand(exec_loop_, command);
}
}
