#include "memcache_client_pool.h"

#include "vbucket_config.h"
#include "evpp/event_loop_thread_pool.h"

namespace evmc {

MemcacheClientPool::~MemcacheClientPool() {
	delete main_loop_;
}

void MemcacheClientPool::Stop(bool wait_thread_exit) {
	if (loop_.running()) {
		loop_.Stop();
	}
    loop_pool_.Stop(wait_thread_exit);
	pthread_rwlock_destroy(&vbucket_config_mutex_);
}

bool MemcacheClientPool::DoReloadConf() {
    // atomic set
    MultiModeVbucketConfigPtr vbconf = std::make_shared<MultiModeVbucketConfig>();
    bool success = vbconf->Load(vbucket_conf_file_.c_str());

    if (success) {
        // std::atomic_store(&vbucket_config_, vbconf);
        {
            pthread_rwlock_rdlock(&vbucket_config_mutex_);
            vbucket_config_ = vbconf;
            pthread_rwlock_unlock(&vbucket_config_mutex_);
        }
        LOG_DEBUG << "DoReloadConf load ok, file=" << vbucket_conf_file_;
    } else {
        LOG_WARN << "DoReloadConf load err, file=" << vbucket_conf_file_;
    }

    return success;
}


MultiModeVbucketConfigPtr MemcacheClientPool::vbucket_config() {
    pthread_rwlock_rdlock(&vbucket_config_mutex_);
	auto ret = vbucket_config_; 
    pthread_rwlock_unlock(&vbucket_config_mutex_);
    return ret;
}

void MemcacheClientPool::MainEventThread() {
	main_loop_->Run();
}
evpp::EventLoop * MemcacheClientPool::main_loop_ = NULL;
bool MemcacheClientPool::Start() {
    if (!DoReloadConf()) {
        return false;
    }

    bool ok = loop_pool_.Start(true);

    if (!ok) {
        return false;
    }

	pthread_rwlock_init(&vbucket_config_mutex_, NULL);
    static evpp::Duration reload_delay(300.0);
    loop_pool_.GetNextLoop()->RunAfter(reload_delay, std::bind(&MemcacheClientPool::DoReloadConf, this));

    auto server_list = vbucket_config_->server_list();

    // 须先构造memc_client_map_数组，再各个元素取地址，否则地址不稳定，可能崩溃
    for (uint32_t i = 0; i < loop_pool_.thread_num(); ++i) {
        memc_client_map_.emplace_back(MemcClientMap());
        evpp::EventLoop* loop = loop_pool_.GetNextLoopWithHash(i);

        for (size_t svr = 0; svr < server_list.size(); ++svr) {
            evpp::TCPClient* tcp_client = new evpp::TCPClient(loop, server_list[svr], "evmc");
            MemcacheClientPtr memc_client(new MemcacheClient(loop, tcp_client, this, timeout_ms_));

            LOG_INFO << "Start new tcp_client=" << tcp_client << " loop=" << i << " server=" << server_list[svr];

            tcp_client->SetConnectionCallback(std::bind(&MemcacheClientPool::OnClientConnection, this,
                                                        std::placeholders::_1, memc_client));
            tcp_client->SetMessageCallback(std::bind(&MemcacheClient::OnResponseData, memc_client,
                                                     std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
            tcp_client->Connect();

            memc_client_map_.back().emplace(server_list[svr], memc_client);
        }
    }

    for (uint32_t i = 0; i < loop_pool_.thread_num(); ++i) {
        evpp::EventLoop* loop = loop_pool_.GetNextLoopWithHash(i);
        loop->set_context(evpp::Any(&memc_client_map_[i]));
    }

    return ok;
}

MemcClientMap* MemcacheClientPool::GetMemcClientMap(int hash) {
    evpp::EventLoop* loop = loop_pool_.GetNextLoopWithHash(hash);
    return evpp::any_cast<MemcClientMap*>(loop->context());
}

void MemcacheClientPool::Set(evpp::EventLoop* caller_loop, const std::string& key, const std::string& value, SetCallback callback) {
    Set(caller_loop, key, value, 0, 0, callback);
}
void MemcacheClientPool::Set(evpp::EventLoop* caller_loop, const std::string& key, const std::string& value, uint32_t flags,
                             uint32_t expire, SetCallback callback) {
    const uint16_t vbucket = vbucket_config()->GetVbucketByKey(key.c_str(), key.size());
    CommandPtr command(new SetCommand(caller_loop, vbucket, key, value, flags, expire, callback));
	LaunchCommand(command);
}

void MemcacheClientPool::Remove(evpp::EventLoop* caller_loop, const std::string& key, RemoveCallback callback) {
    const uint16_t vbucket = vbucket_config()->GetVbucketByKey(key.c_str(), key.size());
    CommandPtr command(new RemoveCommand(caller_loop, vbucket, key, callback));
	LaunchCommand(command);
}

void MemcacheClientPool::Get(evpp::EventLoop* caller_loop, const std::string& key, GetCallback callback) {
    const uint16_t vbucket = vbucket_config()->GetVbucketByKey(key.c_str(), key.size());
    CommandPtr command(new GetCommand(caller_loop, vbucket, key, callback));
	LaunchCommand(command);
}

void MemcacheClientPool::PrefixGet(evpp::EventLoop* caller_loop, const std::string& key, PrefixGetCallback callback) {
    const uint16_t vbucket = vbucket_config()->GetVbucketByKey(key.c_str(), key.size());
    CommandPtr command(new PrefixGetCommand(caller_loop, vbucket, key, callback));
	LaunchCommand(command);
}


void MemcacheClientPool::MultiGet(evpp::EventLoop* caller_loop, const std::vector<std::string>& keys, MultiGetCallback callback) {
    if (keys.size() <= 0) {
        return;
    }
	const std::size_t size = keys.size();
    std::map<uint16_t, std::vector<std::string> > vbucket_keys;

    MultiModeVbucketConfigPtr vbconf = vbucket_config();
	uint16_t vbucket = 0;
	MultiKeyGetHandlerPtr handler(new MultiKeyHandler<MultiGetResult, MultiGetCallback>(callback));
	auto& result = handler->get_result().get_result_map_;
    for (size_t i = 0; i < size; ++i) {
		auto &key = keys[i];
        vbucket = vbconf->GetVbucketByKey(key.c_str(), key.size());
        vbucket_keys[vbucket].emplace_back(key);
		result.emplace(key, GetResult());
    }
	handler->set_vbucket_keys(vbucket_keys);

	auto & vbucket_keys_d = handler->get_vbucket_keys();
    for(auto it : vbucket_keys_d) {
		vbucket = it.first;
		CommandPtr command(new MultiGetCommand(caller_loop, vbucket, handler));
		LaunchCommand(command);
	}
}

void MemcacheClientPool::PrefixMultiGet(evpp::EventLoop* caller_loop, const std::vector<std::string>& keys, PrefixMultiGetCallback callback) {
    if (keys.size() <= 0) {
        return;
    }
	const std::size_t size = keys.size();
    std::map<uint16_t, std::vector<std::string> > vbucket_keys;

    MultiModeVbucketConfigPtr vbconf = vbucket_config();
	uint16_t vbucket = 0;
	PrefixMultiKeyGetHandlerPtr handler(new MultiKeyHandler<PrefixMultiGetResult, PrefixMultiGetCallback>(callback));
	auto& result = handler->get_result().get_result_map_;
    for (size_t i = 0; i < size; ++i) {
		auto &key = keys[i];
        vbucket = vbconf->GetVbucketByKey(key.c_str(), key.size());
        vbucket_keys[vbucket].emplace_back(key);
		result.emplace(key, PrefixGetResultPtr(new PrefixGetResult));
    }
	handler->set_vbucket_keys(vbucket_keys);

	auto & vbucket_keys_d = handler->get_vbucket_keys();
    for(auto it : vbucket_keys_d) {
		vbucket = it.first;
		CommandPtr command(new PrefixMultiGetCommand(caller_loop, vbucket, handler));
		LaunchCommand(command);
	}
}

void MemcacheClientPool::OnClientConnection(const evpp::TCPConnPtr& conn, MemcacheClientPtr memc_client) {
    LOG_INFO << "OnClientConnection conn=" << conn.get() << " memc_conn=" << memc_client->conn().get();

    if (conn && conn->IsConnected()) {
        LOG_INFO << "OnClientConnection connect ok";
        CommandPtr command;

        while (command = memc_client->pop_waiting_command()) {
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

        while (command = memc_client->PopRunningCommand()) {
            if (command->ShouldRetry()) {
                LOG_INFO << "OnClientConnection running retry";
                // LaunchCommand(command);
                command->caller_loop()->RunInLoop(std::bind(&MemcacheClientPool::LaunchCommand, this, command));
            } else {
                command->OnError(ERR_CODE_NETWORK);
            }
        }

        while (command = memc_client->pop_waiting_command()) {
            if (command->ShouldRetry()) {
                LOG_INFO << "OnClientConnection waiting retry";
                // LaunchCommand(command);
                command->caller_loop()->RunInLoop(std::bind(&MemcacheClientPool::LaunchCommand, this, command));
            } else {
                command->OnError(ERR_CODE_NETWORK);
            }
        }
    }
}

void MemcacheClientPool::LaunchCommand(CommandPtr command) {
    const uint32_t thread_hash = next_thread_++;
    loop_pool_.GetNextLoopWithHash(thread_hash)->RunInLoop(
        std::bind(&MemcacheClientPool::DoLaunchCommand, this, thread_hash, command));
}

void MemcacheClientPool::DoLaunchCommand(const int32_t thread_hash, CommandPtr command) {
    MultiModeVbucketConfigPtr vbconf = vbucket_config();
    uint16_t vbucket = command->vbucket_id(); 
	uint16_t server_id = vbconf->SelectServerId(vbucket, command->server_id());

    if (server_id == BAD_SERVER_ID) {
		LOG_ERROR << "bad server id";
        command->OnError(ERR_CODE_DISCONNECT);
        return;
    }


    command->set_server_id(server_id);
    std::string server_addr = vbconf->GetServerAddrById(server_id);
    MemcClientMap* client_map = GetMemcClientMap(thread_hash);

    if (client_map == NULL) {
        command->OnError(ERR_CODE_DISCONNECT);
        LOG_INFO << "DoLaunchCommand thread pool empty";
        return;
    }

    auto it = client_map->find(server_addr);

    if (it == client_map->end()) {
        evpp::TCPClient* tcp_client = new evpp::TCPClient(loop_pool_.GetNextLoopWithHash(thread_hash),
                                                          server_addr, "MemcacheBinaryClient");
        MemcacheClientPtr memc_client(new MemcacheClient(loop_pool_.GetNextLoopWithHash(thread_hash),
                                                         tcp_client, this, timeout_ms_));
        LOG_INFO << "DoLaunchCommand new tcp_client=" << tcp_client << " memc_client=" << memc_client;

        tcp_client->SetConnectionCallback(std::bind(&MemcacheClientPool::OnClientConnection, this,
                                                    std::placeholders::_1, memc_client));
        tcp_client->SetMessageCallback(std::bind(&MemcacheClient::OnResponseData, memc_client,
                                                 std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
        tcp_client->Connect();

        client_map->emplace(server_addr, memc_client);

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

