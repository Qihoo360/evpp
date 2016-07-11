#pragma once

#include "memcache_client.h"
#include "vbucket_config.h"

namespace evmc {

typedef std::map<std::string, MemcacheClientPtr> MemcClientMap;

class MemcacheClientPool {
public:
    friend MemcacheClient;
    MemcacheClientPool(const char* vbucket_conf, int concurrency, int timeout_ms) 
            : vbucket_conf_file_(vbucket_conf), loop_pool_(&loop_, concurrency)
            , timeout_ms_(timeout_ms) {
    }
    virtual ~MemcacheClientPool();

    bool Start();
    void Stop(bool wait_thread_exit);

    void Set(evpp::EventLoop* caller_loop, const std::string& key, const std::string& value, SetCallback callback);
    void Set(evpp::EventLoop* caller_loop, const std::string& key, const std::string& value, uint32_t flags,
                  uint32_t expire, SetCallback callback);

    void Remove(evpp::EventLoop* caller_loop, const std::string& key, RemoveCallback callback);
    void Get(evpp::EventLoop* caller_loop, const std::string& key, GetCallback callback);
    void MultiGet(evpp::EventLoop* caller_loop, const std::vector<std::string>& keys, MultiGetCallback callback);
private:
    // noncopyable
    MemcacheClientPool(const MemcacheClientPool&);
    const MemcacheClientPool& operator=(const MemcacheClientPool&);

private:
    void OnClientConnection(const evpp::TCPConnPtr& conn, MemcacheClientPtr memc_client);
    void LaunchCommand(CommandPtr command);
    void OnReloadConfTimer();
    bool DoReloadConf();
    VbucketConfigPtr vbucket_config();

    MemcClientMap* GetMemcClientMap(int hash);
private:
    void DoLaunchCommand(CommandPtr command);

    thread_local static std::map<std::string, MemcacheClientPtr> memc_clients_;
    std::vector<MemcClientMap> memc_client_map_;

    std::string vbucket_conf_file_;
    evpp::EventLoop loop_;
    evpp::EventLoopThreadPool loop_pool_;
    int timeout_ms_;

    // std::atomic<VbucketConfigPtr> vbucket_config_;
    VbucketConfigPtr vbucket_config_;
    std::mutex vbucket_config_mutex_; // TODO : use rw mutex

    std::atomic_int next_thread_;
};

}


