#pragma once

#include "memcache_client.h"
#include "vbucket_config.h"

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
    MemcacheClientPool(const char* vbucket_conf, int thread_num, int timeout_ms)
        : vbucket_conf_file_(vbucket_conf), loop_pool_(&loop_, thread_num)
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
    void PrefixGet(evpp::EventLoop* caller_loop, const std::string& key, PrefixGetCallback callback);
    void MultiGet(evpp::EventLoop* caller_loop, const std::vector<std::string>& keys, MultiGetCallback callback);
    void PrefixMultiGet(evpp::EventLoop* caller_loop, const std::vector<std::string>& keys, PrefixMultiGetCallback callback);
private:
    // noncopyable
    MemcacheClientPool(const MemcacheClientPool&);
    const MemcacheClientPool& operator=(const MemcacheClientPool&);

private:
    void OnClientConnection(const evpp::TCPConnPtr& conn, MemcacheClientPtr memc_client);
    void LaunchCommand(CommandPtr command);
    bool DoReloadConf();
    MultiModeVbucketConfigPtr vbucket_config();

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
    MultiModeVbucketConfigPtr vbucket_config_;
    std::mutex vbucket_config_mutex_; // TODO : use rw mutex

    std::atomic_int next_thread_;
};

class MultiGetCollector {
public:
    MultiGetCollector(evpp::EventLoop* caller_loop, int count, const MultiGetCallback& cb)
        : caller_loop_(caller_loop), collect_counter_(count), callback_(cb) {}
    void Collect(const MultiGetResult& res) {
		if (res.code == 0) {
			collect_result_.code = res.code; //TODO 0 是什么含义？
		}

        for (auto it = res.get_result_map_.begin(); it != res.get_result_map_.end(); ++it) {
            collect_result_.get_result_map_.insert(*it);
        }

        LOG_DEBUG << "MultiGetCollector count=" << collect_counter_;

        if (--collect_counter_ <= 0) {
            if (caller_loop_) {
                caller_loop_->RunInLoop(std::bind(callback_, collect_result_));
            } else {
                callback_(collect_result_);
            }
        }
    }
private:
    evpp::EventLoop* caller_loop_;
    int collect_counter_;
    MultiGetResult collect_result_;
    MultiGetCallback callback_;
};
typedef std::shared_ptr<MultiGetCollector> MultiGetCollectorPtr;


class PrefixMultiGetCollector {
public:
    PrefixMultiGetCollector(evpp::EventLoop* caller_loop, int count, const PrefixMultiGetCallback& cb)
        : caller_loop_(caller_loop), collect_counter_(count)
		  , collect_result_(new PrefixMultiGetResult()), callback_(cb) {}
    void Collect(const PrefixMultiGetResultPtr res) {
		if (res->code == 0) {
			collect_result_->code = 0; //TODO 0 是什么含义？
		}
		auto& collect_result_map = collect_result_->get_result_map_;
		auto& res_result_map = res->get_result_map_;
        LOG_DEBUG << "PrefixMultiGetCollector keysize=" << res_result_map.size();
		for (auto it = res_result_map.begin(); it != res_result_map.end(); ++it) {
			collect_result_map.insert(std::make_pair(it->first, it->second));
		}
        LOG_DEBUG << "PrefixMultiGetCollector count=" << collect_counter_;
		if (--collect_counter_ <= 0) {
			if (caller_loop_) {
				caller_loop_->RunInLoop(std::bind(callback_, collect_result_));
			} else {
				callback_(collect_result_);
			}
		}
	}
private:
    evpp::EventLoop* caller_loop_;
    int collect_counter_;
    PrefixMultiGetResultPtr collect_result_;
    PrefixMultiGetCallback callback_;
};

typedef std::shared_ptr<PrefixMultiGetCollector> PrefixMultiGetCollectorPtr;
}


