#pragma once

#include "memcache_client.h"
#include "vbucket_config.h"
#include "command.h"
#include "memcache_client_base.h"

namespace evmc {

typedef std::map<std::string, MemcacheClientPtr> MemcClientMap;

class MemcacheClientPool : MemcacheClientBase {
public:
    friend MemcacheClient;

    // @brief
    // @param[in] vbucket_conf - 有三种格式
    //      1. memcached单实例模式，传入的参数应该 "host:port"
    //      2. memcached集群模式，传输的参数可以是vbucket conf url ： "http://host:port/vbucket_conf"
    //      3. memcached集群模式，传输的参数可以是vbucket conf 本地文件： "/the/path/to/vbucket_conf"
    // @param[in] thread_num -
    // @param[in] timeout_ms -
    // @return  -
    MemcacheClientPool(const char* vbucket_conf, int thread_num, int timeout_ms, const char* key_filter = "+")
        : MemcacheClientBase(vbucket_conf), vbucket_conf_file_(vbucket_conf), loop_pool_(&loop_, thread_num)
        , timeout_ms_(timeout_ms), key_filter_(key_filter) {
    }
    virtual ~MemcacheClientPool();

    bool Start();
    void Stop(bool wait_thread_exit);

    void Set(evpp::EventLoop* caller_loop, const std::string& key, const std::string& value, uint32_t flags,
             uint32_t expire, SetCallback callback);
    inline void Set(evpp::EventLoop* caller_loop, const std::string& key, const std::string& value, SetCallback callback) {
        Set(caller_loop, key, value, 0, 0, callback);
    }

    void Remove(evpp::EventLoop* caller_loop, const std::string& key, RemoveCallback callback);
    void Get(evpp::EventLoop* caller_loop, const std::string& key, GetCallback callback);
    void PrefixGet(evpp::EventLoop* caller_loop, const std::string& key, PrefixGetCallback callback);

    void MultiGet(evpp::EventLoop* caller_loop, std::vector<std::string>& keys, MultiGetCallback& callback);
    void RunBackGround(const std::function<void(void)>& fun);
//  void MultiGetImpl(evpp::EventLoop* caller_loop, std::vector<std::string>& keys, MultiGetCallback callback);

    void PrefixMultiGet(evpp::EventLoop* caller_loop, std::vector<std::string>& keys, PrefixMultiGetCallback callback);
    virtual void LaunchCommand(CommandPtr& command);
private:
    // noncopyable
    MemcacheClientPool(const MemcacheClientPool&);
    const MemcacheClientPool& operator=(const MemcacheClientPool&);
    static void MainEventThread();

private:
    void OnClientConnection(const evpp::TCPConnPtr& conn, MemcacheClientPtr memc_client);
    bool DoReloadConf();
    inline MemcClientMap* GetMemcClientMap(evpp::EventLoop* loop) {
        return evpp::any_cast<MemcClientMap*>(loop->context());
    }
private:
    void DoLaunchCommand(evpp::EventLoop* loop, CommandPtr command);

    std::vector<MemcClientMap> memc_client_map_;

    std::string vbucket_conf_file_;
    evpp::EventLoop loop_;
    evpp::EventLoopThreadPool loop_pool_;
    int timeout_ms_;

    MultiModeVbucketConfigPtr vbucket_config_;
    //pthread_rwlock_t vbucket_config_mutex_; // TODO : use rw mutex
    std::string key_filter_;

    std::atomic_int next_thread_;
};

}


