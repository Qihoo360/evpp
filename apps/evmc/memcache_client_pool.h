#pragma once

#include "memcache_client.h"
#include "vbucket_config.h"
#include "command.h"
//#include "objectpool.hpp"

namespace evmc {

typedef std::map<std::string, MemcacheClientPtr> MemcClientMap;
//typedef ObjectPool<Command, MultiGetCommand2> MultiGet2CommandPool; 

class MemcacheClientPool {
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
    MemcacheClientPool(const char* vbucket_conf, int thread_num, int timeout_ms)
        : vbucket_conf_file_(vbucket_conf), loop_pool_(&loop_, thread_num)
        , timeout_ms_(timeout_ms) { 
//		multiget_collector_pool_(new MultiGet2CollectorPool()),
//		multiget_command_pool_(new MultiGet2CommandPool()) {
		//multiget_collector_pool_(NULL),
	    //multiget_command_pool_(NULL) {
    }
    virtual ~MemcacheClientPool();

    bool Start();
    void Stop(bool wait_thread_exit);

    void Set(evpp::EventLoop* caller_loop, const std::string& key, const std::string& value, SetCallback callback);
    void Set(evpp::EventLoop* caller_loop, const std::string& key, const std::string& value, uint32_t flags,
             uint32_t expire, SetCallback callback);

    void Remove(evpp::EventLoop* caller_loop, const std::string& key, RemoveCallback callback);
    void Get(evpp::EventLoop* caller_loop, const std::string& key, GetCallback callback);
    void PrefixGet(evpp::EventLoop* caller_loop, const std::string& key, PrefixGetCallback callback);

    void MultiGet(evpp::EventLoop* caller_loop, const std::vector<std::string>& keys, MultiGetCallback callback);

    void PrefixMultiGet(evpp::EventLoop* caller_loop, const std::vector<std::string>& keys, PrefixMultiGetCallback callback);
private:
    // noncopyable
    MemcacheClientPool(const MemcacheClientPool&);
    const MemcacheClientPool& operator=(const MemcacheClientPool&);
	static void MainEventThread();

private:
    void OnClientConnection(const evpp::TCPConnPtr& conn, MemcacheClientPtr memc_client);
    void LaunchCommand(CommandPtr command);
    bool DoReloadConf();
    MultiModeVbucketConfigPtr vbucket_config();

    MemcClientMap* GetMemcClientMap(int hash);
private:
    void DoLaunchCommand(const int32_t thread_hash, CommandPtr command);

    std::vector<MemcClientMap> memc_client_map_;

    std::string vbucket_conf_file_;
    evpp::EventLoop loop_;
    static evpp::EventLoop *main_loop_;
    evpp::EventLoopThreadPool loop_pool_;
    int timeout_ms_;

    // std::atomic<VbucketConfigPtr> vbucket_config_;
    MultiModeVbucketConfigPtr vbucket_config_;
    pthread_rwlock_t vbucket_config_mutex_; // TODO : use rw mutex

    std::atomic_int next_thread_;
	//MultiGet2CollectorPool	* multiget_collector_pool_;
	//MultiGet2CommandPool * multiget_command_pool_;
};

}


