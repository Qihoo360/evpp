#pragma once

#include "mctypes.h"

#include <queue>

#include "evpp/tcp_conn.h"
#include "evpp/tcp_client.h"

#include "evpp/libevent_watcher.h"
#include "evpp/event_loop.h"
#include "evpp/event_loop_thread_pool.h"

#include "command.h"

namespace evmc {
class BinaryCodec;
class MemcacheClientPool;

class MemcacheClient : public std::enable_shared_from_this<MemcacheClient> {
public:
    MemcacheClient(evpp::EventLoop* evloop, evpp::TCPClient* tcp_client, MemcacheClientPool* mcpool = NULL, int timeout_ms = 249)
            : id_seq_(0), exec_loop_(evloop), tcp_client_(tcp_client)
            , mc_pool_(mcpool), timeout_(timeout_ms / 1000.0), codec_(NULL) {
    }
    virtual ~MemcacheClient();

    EventLoopPtr exec_loop() const { return exec_loop_; }

    void PushRunningCommand(CommandPtr cmd);

    CommandPtr PopRunningCommand();
    CommandPtr peek_running_command() {
        if (running_command_.empty()) {
            return CommandPtr(); 
        }
        return CommandPtr(running_command_.front());
    }

    void push_waiting_command(CommandPtr cmd) {
        if (cmd) {
            cmd->set_id(next_id());
            waiting_command_.push(cmd);
        }
    }
    CommandPtr pop_waiting_command();

    evpp::TCPConnPtr conn() const { return tcp_client_->conn();}
    uint32_t next_id() { return ++id_seq_; }

    void EmbededGet(const char* key, GetCallback callback) {
      //CommandPtr command(new GetCommand(EventLoopPtr(), key, callback));
      //command->Launch(this);
    }

    void OnResponseData(const evpp::TCPConnPtr& tcp_conn,
           evpp::Buffer* buf,
           evpp::Timestamp ts);
    void OnPacketTimeout(uint32_t cmd_id);

private:
    // noncopyable
    MemcacheClient(const MemcacheClient&);
    const MemcacheClient& operator=(const MemcacheClient&);
private:
    // std::atomic_uint id_seq_;
    uint32_t id_seq_;

    EventLoopPtr exec_loop_;
    evpp::TCPClient* tcp_client_;
    MemcacheClientPool* mc_pool_;
    evpp::Duration timeout_;

    // TimerEventPtr cmd_timer_;
    evpp::InvokeTimerPtr cmd_timer_;
    uint32_t timer_cmd_id_;

    BinaryCodec * codec_;

    std::queue<CommandPtr> running_command_;
    std::queue<CommandPtr> waiting_command_;
};


}

