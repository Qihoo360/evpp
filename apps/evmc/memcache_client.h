#pragma once

#include <queue>

#include "evpp/tcp_conn.h"
#include "evpp/tcp_client.h"

#include "evpp/event_watcher.h"
#include "evpp/event_loop.h"
#include "evpp/event_loop_thread_pool.h"

#include "mctypes.h"
#include "command.h"

namespace evmc {
class BinaryCodec;
class MemcacheClientPool;

class MemcacheClient : public std::enable_shared_from_this<MemcacheClient> {
public:
    MemcacheClient(evpp::EventLoop* evloop, evpp::TCPClient* tcp_client, MemcacheClientPool* mcpool = nullptr, const int timeout_ms = 249)
        : exec_loop_(evloop), tcp_client_(tcp_client)
        , mc_pool_(mcpool), timeout_(timeout_ms), codec_(nullptr) {
        send_next_id_ = 0;
        recv_next_id_ = 0;
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

    inline evpp::TCPConnPtr conn() const {
        return tcp_client_->conn();
    }

    inline uint32_t next_send_id() {
        return send_next_id_++;
    }

    inline uint32_t send_id() {
        //return send_next_id_.load(std::memory_order_acquire);
        return send_next_id_;
    }

    inline uint32_t recv_id() {
        //return recv_next_id_.load(std::memory_order_acquire);
        return recv_next_id_;
    }

    inline uint32_t next_recv_id() {
        return recv_next_id_++;
    }

    void OnConnectTimeout(uint32_t cmd_id);
    void OnResponseData(const evpp::TCPConnPtr& tcp_conn,
                        evpp::Buffer* buf);
    void OnPacketTimeout(const uint32_t cmd_id);
    void OnTimeout(const uint32_t recv_id);
    void Launch(CommandPtr& cmd);
    void SendData();
    void OnClientConnection(const evpp::TCPConnPtr& conn);

private:
    // noncopyable
    MemcacheClient(const MemcacheClient&);
    const MemcacheClient& operator=(const MemcacheClient&);
private:
    std::atomic_uint send_next_id_;
    std::atomic_uint recv_next_id_;

    //timer checkpoint
    int64_t recv_checkpoint_{-1};
    //int64_t send_checkpoint_{-1};

    evpp::EventLoop* exec_loop_;
    evpp::TCPClient* tcp_client_;
    MemcacheClientPool* mc_pool_;
    evpp::Buffer send_buf_;
    int timeout_;

    BinaryCodec* codec_;
    std::queue<CommandPtr> running_command_;
};


}

