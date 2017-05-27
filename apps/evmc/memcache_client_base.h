#pragma once

#include <map>
#include <mutex>


#include "evmc/config.h"

#include "evpp/event_loop_thread_pool.h"

#include "vbucket_config.h"
#include "memcache_client.h"

namespace evmc {
class MemcacheClientBase {
public:
    virtual ~MemcacheClientBase();
    MemcacheClientBase(const char* vbucket_conf);
    MultiModeVbucketConfig*  vbucket_config();
    virtual void LaunchCommand(CommandPtr& command) = 0;
protected:
    bool Start(bool is_reload = false);

    void Stop();
    void BuilderMemClient(evpp::EventLoop* loop, std::string& server, std::map<std::string, MemcacheClientPtr>& client_map, const int timeout_ms);
    void OnClientConnection(const evpp::TCPConnPtr& conn, MemcacheClientPtr memc_client);
private:
    void DoReloadConf();
    void LoadThread();
private:
    std::string vbucket_conf_;
    evpp::EventLoop*  load_loop_;
    std::thread* load_thread_;
    MultiModeVbucketConfig* vbconf_cur_, *vbconf_1_, *vbconf_2_;
    std::mutex vbucket_config_mutex_;
};
}
