#include "memcache_client_base.h"
#include "vbucket_config.h"
#include "evpp/event_loop_thread_pool.h"
#include "likely.h"
#include <mutex>

namespace evmc {

MemcacheClientBase::~MemcacheClientBase() {
    delete load_loop_;
    delete load_thread_;
    delete vbconf_1_;
    delete vbconf_2_;
}

void MemcacheClientBase::Stop() {
    if (load_loop_) {
        load_loop_->Stop();
    }
    if (load_thread_) {
        load_thread_->join();
    }
}

void MemcacheClientBase::DoReloadConf() {
    MultiModeVbucketConfig* vbconf = nullptr;
    if (vbconf_cur_ == vbconf_1_) {
        vbconf = vbconf_2_;
        vbconf_2_->clear();
    } else {
        vbconf = vbconf_1_;
        vbconf_1_->clear();
    }
    bool success = vbconf->Load(vbucket_conf_.c_str());

    if (success) {
        std::lock_guard<std::mutex> lck(vbucket_config_mutex_);
        vbconf_cur_ = vbconf;
        LOG_DEBUG << "DoReloadConf load ok, file=" << vbucket_conf_;
    } else {
        LOG_WARN << "DoReloadConf load err, file=" << vbucket_conf_;
    }
    return;
}


MultiModeVbucketConfig*  MemcacheClientBase::vbucket_config() {
    std::lock_guard<std::mutex> lck(vbucket_config_mutex_);
    auto ret = vbconf_cur_;
    return ret;
}

void MemcacheClientBase::LoadThread() {
    load_loop_->Run();
    LOG_ERROR << "load thread exit...";
}


void MemcacheClientBase::BuilderMemClient(evpp::EventLoop* loop, std::string& server, std::map<std::string, MemcacheClientPtr>& client_map, const int timeout_ms) {
    evpp::TCPClient* tcp_client = new evpp::TCPClient(loop, server, "evmc");
    MemcacheClientPtr memc_client = std::make_shared<MemcacheClient>(loop, tcp_client, this, timeout_ms);

    LOG_INFO << "Start new tcp_client=" << tcp_client << " server=" << server << " timeout=" << timeout_ms;

    tcp_client->SetConnectionCallback(std::bind(&MemcacheClientBase::OnClientConnection, this,
                                                std::placeholders::_1, memc_client));
    tcp_client->SetMessageCallback(std::bind(&MemcacheClient::OnResponseData, memc_client,
                                             std::placeholders::_1, std::placeholders::_2));
    tcp_client->Connect();
    client_map.emplace(server, memc_client);
}


MemcacheClientBase::MemcacheClientBase(const char* vbucket_conf): vbucket_conf_(vbucket_conf), load_loop_(nullptr)
    , load_thread_(nullptr), vbconf_cur_(nullptr), vbconf_1_(new MultiModeVbucketConfig()), vbconf_2_(new MultiModeVbucketConfig()) {
    assert(vbconf_1_);
    assert(vbconf_2_);
}

bool MemcacheClientBase::Start(bool is_reload) {
    if (is_reload) {
        load_loop_ = new evpp::EventLoop();
        assert(load_loop_);
        if (!vbconf_2_->Load(vbucket_conf_.c_str())) {
            LOG_ERROR << "load error .file=" << vbucket_conf_;
            delete load_loop_;
            return false;
        }
        vbconf_cur_ = vbconf_2_;
        /*load_thread_ = new std::thread(MemcacheClientBase::LoadThread, this);
        assert(load_thread_);
        evpp::Duration reload_delay(120.0);
        while(!load_loop_->running()) {
            usleep(1000);
        }
        load_loop_->RunEvery(reload_delay, std::bind(&MemcacheClientBase::DoReloadConf, this));*/
    }
    return true;
}

void MemcacheClientBase::OnClientConnection(const evpp::TCPConnPtr& conn, MemcacheClientPtr memc_client) {
    LOG_INFO << "OnClientConnection conn=" << conn.get() << " memc_conn=" << memc_client->conn().get();

    if (conn && conn->IsConnected()) {
        LOG_INFO << "OnClientConnection connect ok";
        CommandPtr command;

        while (command = memc_client->PopWaitingCommand()) {
            memc_client->PushRunningCommand(command);
            command->Launch(memc_client);
        }
    } else {
        if (conn) {
            LOG_INFO << "Disconnected from " << conn->remote_addr();
        } else {
            LOG_INFO << "Connect init error";
        }

        CommandPtr command;

        while (command = memc_client->PopRunningCommand()) {
            if (command->ShouldRetry()) {
                command->set_id(0);
                command->set_server_id(command->server_id());
                memc_client->exec_loop()->RunInLoop(std::bind(&MemcacheClientBase::LaunchCommand, this, command));
            } else {
                command->OnError(ERR_CODE_NETWORK);
            }
        }

        while (command = memc_client->PopWaitingCommand()) {
            if (command->ShouldRetry()) {
                command->set_id(0);
                command->set_server_id(command->server_id());
                memc_client->exec_loop()->RunInLoop(std::bind(&MemcacheClientBase::LaunchCommand, this, command));
            } else {
                command->OnError(ERR_CODE_NETWORK);
            }
        }
    }
}

}


