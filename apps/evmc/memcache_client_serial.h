#pragma once

#include <queue>

#include "mctypes.h"
#include "vbucket_config.h"
#include "memcache_client_base.h"
#include "evpp/event_loop_thread_pool.h"

namespace evmc {

class MemcacheClientSerial : public MemcacheClientBase {
public:
    MemcacheClientSerial(const char* vbucket_conf, int timeout_ms)
        : MemcacheClientBase(vbucket_conf), server_(vbucket_conf), timeout_ms_(timeout_ms), loop_(nullptr), memclient_(MemcacheClientPtr()), id_seq_(0) {
    }

    virtual ~MemcacheClientSerial();
    void Stop();
    bool Start(evpp::EventLoop* loop);
    void Set(const std::string& key, const std::string& value, uint32_t flags,
             uint32_t expire, SetCallback callback);
    inline void Set(const std::string& key, const std::string& value, SetCallback callback) {
        Set(key, value, 0, 0, callback);
    }
    void Remove(const std::string& key, RemoveCallback callback);
    void Get(const std::string& key, GetCallback callback);
    void MultiGet(const std::vector<std::string>& keys, MultiGetCallback callback);

    virtual void LaunchCommand(CommandPtr& command);
private:
    inline uint32_t next_id() {
        return ++id_seq_;
    }
private:
    std::string server_;
    int32_t  timeout_ms_;
    evpp::EventLoop* loop_;
    MemcacheClientPtr memclient_;
    uint32_t id_seq_;
};
}
