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

void MemcacheClientPool::MultiGet(EventLoopPtr caller_loop, const std::vector<std::string>& keys, MultiGetCallback callback) {
    if (keys.size() <= 0) {
        return;
    }
    uint16_t vbucket = vbucket_config_->GetVbucketByKey(keys[0].c_str(), keys[0].size());
    CommandPtr command(new MultiGetCommand(caller_loop, vbucket, keys, callback));
    caller_loop->RunInLoop(std::bind(&MemcacheClientPool::LaunchCommand, this, command));
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
            command->OnError(ERR_CODE_NETWORK);
        }

        while(command = memc_client->pop_waiting_command()) {
            command->OnError(ERR_CODE_NETWORK);
        }
    }
}

void MemcacheClientPool::LaunchCommand(CommandPtr command) {
    LOG_INFO << "LaunchCommand";
    loop_pool_.GetNextLoopWithHash(command->thread_hash())->RunInLoop(
            std::bind(&MemcacheClientPool::DoLaunchCommand, this, command));
}

void MemcacheClientPool::DoLaunchCommand(CommandPtr command) {
    auto it = memc_clients_.find(command->ServerAddr());

    if (it == memc_clients_.end()) {
        evpp::TCPClient * tcp_client = new evpp::TCPClient(loop_pool_.GetNextLoopWithHash(command->thread_hash()),
                command->ServerAddr(), "MemcacheBinaryClient");
        MemcacheClientPtr memc_client(new MemcacheClient(loop_pool_.GetNextLoopWithHash(command->thread_hash()),
                tcp_client, timeout_ms_));
        LOG_INFO << "DoLaunchCommand new tcp_client=" << tcp_client << " memc_client=" << memc_client;

        tcp_client->SetConnectionCallback(std::bind(&MemcacheClientPool::OnClientConnection,
                    std::placeholders::_1, memc_client));
        tcp_client->SetMesageCallback(std::bind(&MemcacheClient::OnResponseData, memc_client,
                    std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
        tcp_client->Connect();

        memc_clients_[command->ServerAddr()] = memc_client;

        memc_client->push_waiting_command(command);
        return;
    }

    if (!it->second->conn() || it->second->conn()->status() == evpp::TCPConn::kConnecting) {
        it->second->push_waiting_command(command);
    } else if (it->second->conn()->IsConnected()) {
        it->second->PushRunningCommand(command);
        command->Launch(it->second);
    } else {
        command->OnError(ERR_CODE_DISCONNECT);
    }
}

}

