#pragma once

#include "memcache_client.h"
#include "vbucket_config.h"
#include "command.h"

namespace evmc {

typedef std::map<std::string, MemcacheClientPtr> MemcClientMap;

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
    MemcacheClientPool(const char* vbucket_conf, int thread_num, int timeout_ms, const char* key_filter = "+")
        : vbucket_conf_file_(vbucket_conf), loop_pool_(&loop_, thread_num)
        , timeout_ms_(timeout_ms), key_filter_(key_filter) {
        vbucket_config_.reset(new MultiModeVbucketConfig());
        assert(vbucket_config_ == true);
    }

    inline MultiModeVbucketConfig* vbucket_config() {
        return vbucket_config_.get();
    }

    virtual ~MemcacheClientPool();
    void BuilderMemClient(evpp::EventLoop* loop, std::string& server, std::map<std::string, MemcacheClientPtr>& client_map, const int timeout_ms);

    bool Start();
    void Stop(bool wait_thread_exit);


    void MultiGet(evpp::EventLoop* caller_loop, const std::vector<std::string>& keys, MultiGetCallback& callback);
    str2strFuture MultiGet(const std::vector<std::string>& keys);

    void Set(evpp::EventLoop* caller_loop, const std::string& key, const std::string& value, SetCallback& callback);
    intFuture Set(const std::string& key,  const std::string& value);

    void Remove(evpp::EventLoop* caller_loop, const std::string& key, RemoveCallback& callback);
    intFuture Remove(const std::string& key);

    void Get(evpp::EventLoop* caller_loop, const std::string& key, GetCallback& callback);
    stringFuture Get(const std::string& key);

    void PrefixMultiGet(evpp::EventLoop* caller_loop, std::vector<std::string>& keys, PrefixMultiGetCallback& callback);
    str2mapFuture PrefixMultiGet(const std::vector<std::string>& keys);

    void PrefixGet(evpp::EventLoop* caller_loop, const std::string& key, PrefixGetCallback& callback);
    str2mapFuture PrefixGet(const std::string& key);


    void ReLaunchCommand(evpp::EventLoop* loop, CommandPtr& command);
    void RunBackGround(const std::function<void(void)>& fun);
private:
    // noncopyable
    MemcacheClientPool(const MemcacheClientPool&) = delete;
    const MemcacheClientPool& operator=(const MemcacheClientPool&) = delete;
    inline MemcClientMap* GetMemcClientMap(evpp::EventLoop* loop) {
        return evpp::any_cast<MemcClientMap*>(loop->context());
    }
    void LaunchCommand(CommandPtr& command);
    void DoLaunchCommand(evpp::EventLoop* loop, CommandPtr command);
    template<class CmdType, class RetType>
    folly::Future<RetType> LaunchOneKeyCommand(const std::string& key, const std::string& value);
    template <class CmdType, class RetType>
    void LaunchMultiKeyCommand(const std::vector<std::string>& keys, std::vector<folly::Future<RetType>>& futs);
    template <class RetType>
    folly::Future<RetType> CollectAllFuture(std::vector<folly::Future<RetType>>& futs);

private:

    std::vector<MemcClientMap> memc_client_map_;

    std::string vbucket_conf_file_;
    evpp::EventLoop loop_;
    evpp::EventLoopThreadPool loop_pool_;
    int timeout_ms_;

    MultiModeVbucketConfigPtr vbucket_config_;
    std::string key_filter_;
};

}


