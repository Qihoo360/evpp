#pragma once

#include "mctypes.h"

#include <queue>

#include "evpp/tcp_conn.h"
#include "evpp/tcp_client.h"

#include "evpp/libevent_watcher.h"
#include "evpp/event_loop.h"
#include "evpp/event_loop_thread_pool.h"

#include "command.h"
#include "memcache_client_base.h"

namespace evmc {
class BinaryCodec;
class MemcacheClientBase;

class MemcacheClient : public std::enable_shared_from_this<MemcacheClient> {
public:
    MemcacheClient(evpp::EventLoop* evloop, evpp::TCPClient* tcp_client, MemcacheClientBase* mcpool = nullptr, const int timeout_ms = 249)
        : id_seq_(0), exec_loop_(evloop), tcp_client_(tcp_client)
        , mc_pool_(mcpool), timeout_(timeout_ms * 1000 * 1000), codec_(nullptr), timer_canceled_(true), con_timer_canceled_(true) {
    }
    virtual ~MemcacheClient();

    inline evpp::EventLoop* exec_loop() const {
        return exec_loop_;
    }

    void PushRunningCommand(CommandPtr& cmd);

    CommandPtr PopRunningCommand();
    CommandPtr PeekRunningCommand() {
        if (UNLIKELY(running_command_.empty())) {
            return CommandPtr();
        }
        return CommandPtr(running_command_.front());
    }

    void PushWaitingCommand(CommandPtr& cmd);

    CommandPtr PopWaitingCommand();

    inline evpp::TCPConnPtr conn() const {
        return tcp_client_->conn();
    }
    inline uint32_t next_id() {
        return ++id_seq_;
    }

    void OnConnectTimeout(uint32_t cmd_id);
    void OnResponseData(const evpp::TCPConnPtr& tcp_conn,
                        evpp::Buffer* buf);
    void OnPacketTimeout(uint32_t cmd_id);

private:
    // noncopyable
    MemcacheClient(const MemcacheClient&);
    const MemcacheClient& operator=(const MemcacheClient&);
private:
    uint32_t id_seq_;

    evpp::EventLoop* exec_loop_;
    evpp::TCPClient* tcp_client_;
    MemcacheClientBase* mc_pool_;
    evpp::Duration timeout_;

    evpp::InvokeTimerPtr cmd_timer_;
    evpp::InvokeTimerPtr con_cmd_timer_;
    evpp::InvokeTimerPtr cmd_timer_bakup_;

    BinaryCodec* codec_;
    bool  timer_canceled_;
    bool  con_timer_canceled_;

    std::queue<CommandPtr> running_command_;
    std::queue<CommandPtr> waiting_command_;
};


}

