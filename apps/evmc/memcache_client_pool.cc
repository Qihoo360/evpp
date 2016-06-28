#include "memcache_client_pool.h"

#include "vbucket_config.h"

namespace evmc {

thread_local std::map<std::string, MemcacheClientPtr> MemcacheClientPool::memc_clients_;

MemcacheClientPool::~MemcacheClientPool() {
    loop_pool_.Stop(true);
    if (vbucket_config_) {
        delete vbucket_config_;
    }
}

bool MemcacheClientPool::Start() {
    vbucket_config_ = new VbucketConfig();
    if (!vbucket_config_->Load(vbucket_conf_file_.c_str())) {
        return false;
    }
    return loop_pool_.Start(true);
}

void MemcacheClientPool::Set(EventLoopPtr caller_loop, const std::string& key, const std::string& value, SetCallback callback) {
    Set(caller_loop, key.c_str(), value.c_str(), value.size(), 0, 0, callback);
}
void MemcacheClientPool::Set(EventLoopPtr caller_loop, const char* key, const char* value, size_t val_len, uint32_t flags,
              uint32_t expire, SetCallback callback) {
    uint16_t vbucket = vbucket_config_->GetVbucketByKey(key, strlen(key));
    CommandPtr command(new SetCommand(caller_loop, vbucket, key, value, val_len, flags, expire, callback));

    caller_loop->RunInLoop(std::bind(&MemcacheClientPool::LaunchCommand, this, command));
}

void MemcacheClientPool::Remove(EventLoopPtr caller_loop, const char* key, RemoveCallback callback) {
    uint16_t vbucket = vbucket_config_->GetVbucketByKey(key, strlen(key));
    CommandPtr command(new RemoveCommand(caller_loop, vbucket, key, callback));

    caller_loop->RunInLoop(std::bind(&MemcacheClientPool::LaunchCommand, this, command));
}

void MemcacheClientPool::Get(EventLoopPtr caller_loop, const char* key, GetCallback callback) {
    uint16_t vbucket = vbucket_config_->GetVbucketByKey(key, strlen(key));
    CommandPtr command(new GetCommand(caller_loop, vbucket, key, callback));

    caller_loop->RunInLoop(std::bind(&MemcacheClientPool::LaunchCommand, this, command));
}

class MultiGetCollector {
public:
    MultiGetCollector(EventLoopPtr caller_loop, int count, const MultiGetCallback& cb)
            : caller_loop_(caller_loop), collect_counter_(count), callback_(cb) {}
    void Collect(const MultiGetResult& res) {
        collect_result_.code = res.code; // TODO : 部分失败时，code如何指定?
        for(auto it = res.get_result_map_.begin(); it != res.get_result_map_.end(); ++it) {
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
    EventLoopPtr caller_loop_;
    int collect_counter_;
    MultiGetResult collect_result_;
    MultiGetCallback callback_;
};
typedef std::shared_ptr<MultiGetCollector> MultiGetCollectorPtr;

void MemcacheClientPool::MultiGet(EventLoopPtr caller_loop, const std::vector<std::string>& keys, MultiGetCallback callback) {
    if (keys.size() <= 0) {
        return;
    }
    uint32_t thread_hash = rand();
    std::map<uint16_t, std::vector<std::string> > vbucket_keys;

    for(size_t i = 0; i < keys.size(); ++i) {
        uint16_t vbucket = vbucket_config_->GetVbucketByKey(keys[i].c_str(), keys[i].size());
        vbucket_keys[vbucket].push_back(keys[i]);
    }
    MultiGetCollectorPtr collector(new MultiGetCollector(caller_loop, vbucket_keys.size(), callback));

    for(auto it = vbucket_keys.begin(); it != vbucket_keys.end(); ++it) {
        CommandPtr command(new MultiGetCommand(caller_loop, it->first, thread_hash, it->second,
            std::bind(&MultiGetCollector::Collect, collector, std::placeholders::_1)));
        caller_loop->RunInLoop(std::bind(&MemcacheClientPool::LaunchCommand, this, command));
    }
}

void MemcacheClientPool::OnClientConnection(const evpp::TCPConnPtr& conn, MemcacheClientPtr memc_client) {
    LOG_INFO << "OnClientConnection conn=" << conn.get() << " memc_conn=" << memc_client->conn().get();
    if (conn && conn->IsConnected()) {
        LOG_INFO << "OnClientConnection connect ok";
        CommandPtr command;
        while(command = memc_client->pop_waiting_command()) {
            memc_client->PushRunningCommand(command);
            command->Launch(memc_client);
            // command->caller_loop()->AssertInLoopThread();
        }
    } else {
        if (conn) {
            LOG_INFO << "Disconnected from " << conn->remote_addr();
        } else {
            LOG_INFO << "Connect init error";
        }

        CommandPtr command;
        while(command = memc_client->PopRunningCommand()) {
            if (command->ShouldRetry()) {
                LOG_INFO << "OnClientConnection running retry";
                LaunchCommand(command);
            } else {
                command->OnError(ERR_CODE_NETWORK);
            }
        }

        while(command = memc_client->pop_waiting_command()) {
            if (command->ShouldRetry()) {
                LOG_INFO << "OnClientConnection waiting retry";
                LaunchCommand(command);
            } else {
                command->OnError(ERR_CODE_NETWORK);
            }
        }
    }
}

void MemcacheClientPool::LaunchCommand(CommandPtr command) {
    LOG_INFO << "LaunchCommand";
    loop_pool_.GetNextLoopWithHash(command->thread_hash())->RunInLoop(
            std::bind(&MemcacheClientPool::DoLaunchCommand, this, command));
}

void MemcacheClientPool::DoLaunchCommand(CommandPtr command) {
    uint16_t vbucket = command->vbucket_id();
    uint16_t server_id = vbucket_config_->SelectServerId(vbucket, command->server_id());
    if (server_id == BAD_SERVER_ID) {
         command->OnError(ERR_CODE_DISCONNECT);
         return;
    }
    command->set_server_id(server_id);
    std::string server_addr = vbucket_config_->GetServerAddrById(server_id);
    auto it = memc_clients_.find(server_addr);

    if (it == memc_clients_.end()) {
        evpp::TCPClient * tcp_client = new evpp::TCPClient(loop_pool_.GetNextLoopWithHash(command->thread_hash()),
                server_addr, "MemcacheBinaryClient");
        MemcacheClientPtr memc_client(new MemcacheClient(loop_pool_.GetNextLoopWithHash(command->thread_hash()),
                tcp_client, this, timeout_ms_));
        LOG_INFO << "DoLaunchCommand new tcp_client=" << tcp_client << " memc_client=" << memc_client;

        tcp_client->SetConnectionCallback(std::bind(&MemcacheClientPool::OnClientConnection, this,
                    std::placeholders::_1, memc_client));
        tcp_client->SetMessageCallback(std::bind(&MemcacheClient::OnResponseData, memc_client,
                    std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
        tcp_client->Connect();

        memc_clients_[server_addr] = memc_client;

        memc_client->push_waiting_command(command);
        return;
    }

    if (!it->second->conn() || it->second->conn()->status() == evpp::TCPConn::kConnecting) {
        it->second->push_waiting_command(command);
    } else if (it->second->conn()->IsConnected()) {
        it->second->PushRunningCommand(command);
        command->Launch(it->second);
    } else {
        if (command->ShouldRetry()) {
            LOG_INFO << "OnClientConnection disconnect retry";
            LaunchCommand(command);
        } else {
            command->OnError(ERR_CODE_DISCONNECT);
        }
    }
}

}

