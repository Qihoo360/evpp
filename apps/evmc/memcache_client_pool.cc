#include "memcache_client_pool.h"

#include "vbucket_config.h"
#include "evpp/event_loop_thread_pool.h"
#include "likely.h"

namespace evmc {

#define GET_FILTER_KEY_POS(name, key) \
    name = key.find(key_filter_[0]); \
    if (name == std::string::npos) { \
        name = key.size(); \
    }

#define SET_SERVERID(vbucket,command) \
    MultiModeVbucketConfig* vbconf = vbucket_config(); \
    uint16_t server_id = vbconf->SelectServerId(vbucket, BAD_SERVER_ID); \
    command->set_server_id(server_id); \

    MemcacheClientPool::~MemcacheClientPool() {
        if (loop_pool_.IsRunning()) {
            Stop(true);
        }
    }

    void MemcacheClientPool::Stop(bool wait_thread_exit) {
        loop_pool_.Stop(wait_thread_exit);
        MemcacheClientBase::Stop();
    }


    bool MemcacheClientPool::Start() {

        bool ok = loop_pool_.Start(true);

        if (UNLIKELY(!ok)) {
            LOG_ERROR << "loop pool start failed";
            return false;
        }

        if (!MemcacheClientBase::Start(true)) {
            loop_pool_.Stop(true);
            LOG_ERROR << "vbucket init failed";
            return false;
        }
        auto server_list = vbucket_config()->server_list();

        // 须先构造memc_client_map_数组，再各个元素取地址，否则地址不稳定，可能崩溃
        for (uint32_t i = 0; i < loop_pool_.thread_num(); ++i) {
            memc_client_map_.emplace_back(MemcClientMap());
            evpp::EventLoop* loop = loop_pool_.GetNextLoopWithHash(i);

            for (size_t svr = 0; svr < server_list.size(); ++svr) {
                auto& client_map = memc_client_map_.back();
                BuilderMemClient(loop, server_list[svr], client_map, timeout_ms_);
            }
        }

        for (uint32_t i = 0; i < loop_pool_.thread_num(); ++i) {
            evpp::EventLoop* loop = loop_pool_.GetNextLoopWithHash(i);
            loop->set_context(evpp::Any(&memc_client_map_[i]));
        }

        return ok;
    }

    void MemcacheClientPool::Set(evpp::EventLoop* caller_loop, const std::string& key, const std::string& value, uint32_t flags,
                                 uint32_t expire, SetCallback callback) {

        std::size_t pos = 0;
        GET_FILTER_KEY_POS(pos, key)
        const uint16_t vbucket = vbucket_config()->GetVbucketByKey(key.c_str(), pos);
        CommandPtr command = std::make_shared<SetCommand>(caller_loop, vbucket, key, value, flags, expire, callback);
        SET_SERVERID(vbucket, command)
        LaunchCommand(command);
    }

    void MemcacheClientPool::Remove(evpp::EventLoop* caller_loop, const std::string& key, RemoveCallback callback) {
        std::size_t pos = 0;
        GET_FILTER_KEY_POS(pos, key)
        const uint16_t vbucket = vbucket_config()->GetVbucketByKey(key.c_str(), pos);
        CommandPtr command = std::make_shared<RemoveCommand>(caller_loop, vbucket, key, callback);
        SET_SERVERID(vbucket, command)
        LaunchCommand(command);
    }

    void MemcacheClientPool::Get(evpp::EventLoop* caller_loop, const std::string& key, GetCallback callback) {
        std::size_t pos = 0;
        GET_FILTER_KEY_POS(pos, key)
        const uint16_t vbucket = vbucket_config()->GetVbucketByKey(key.c_str(), pos);
        CommandPtr command = std::make_shared<GetCommand>(caller_loop, vbucket, key, callback);
        SET_SERVERID(vbucket, command)
        LaunchCommand(command);
    }

    void MemcacheClientPool::PrefixGet(evpp::EventLoop* caller_loop, const std::string& key, PrefixGetCallback callback) {
        std::size_t pos = 0;
        GET_FILTER_KEY_POS(pos, key)
        const uint16_t vbucket = vbucket_config()->GetVbucketByKey(key.c_str(), pos);
        CommandPtr command = std::make_shared<PrefixGetCommand>(caller_loop, vbucket, key, callback);
        SET_SERVERID(vbucket, command)
        LaunchCommand(command);
    }

    void MemcacheClientPool::RunBackGround(const std::function<void(void)>& fun) {
        auto loop = loop_pool_.GetNextLoopWithHash(rand());
        loop->RunInLoop(fun);
    }

    void MemcacheClientPool::MultiGet(evpp::EventLoop* caller_loop, std::vector<std::string>& keys, MultiGetCallback& callback) {
        if (UNLIKELY(keys.size() <= 0)) {
            MultiGetResult result;
            caller_loop->RunInLoop(std::bind(callback, std::move(result)));
            return;
        }
        const std::size_t size = keys.size();
        std::map<uint16_t, IdInfo> serverid_keys;

        MultiModeVbucketConfig* vbconf = vbucket_config();
        uint16_t vbucket = 0;
        uint16_t server_id = 0;
        MultiKeyGetHandlerPtr handler = std::make_shared<MultiKeyHandler<MultiGetResult, MultiGetCallback> > (callback);
        auto& result = handler->get_result();
        std::size_t pos = 0;
        std::vector<int> serverid_table(vbconf->server_list().size(), -1);

        for (size_t i = 0; i < size; ++i) {
            auto& key = keys[i];
            GET_FILTER_KEY_POS(pos, key)
            vbucket = vbconf->GetVbucketByKey(key.c_str(), pos);
            server_id = vbconf->SelectServerFirstId(vbucket);
            if (serverid_table[server_id] == -1) {
                serverid_table[server_id] = vbconf->SelectServerId(vbucket, BAD_SERVER_ID);
            }
            server_id = serverid_table[server_id];

            auto& item = serverid_keys[server_id];
            item.keys.emplace_back(key);
            item.vbuckets.emplace_back(vbucket);
            result.emplace(std::move(key), std::move(GetResult()));
        }
        handler->set_serverid_keys(serverid_keys);

        auto& serverid_keys_d = handler->get_serverid_keys();
        for (auto& it : serverid_keys_d) {
            server_id = it.first;
            CommandPtr command = std::make_shared<MultiGetCommand>(caller_loop, server_id, handler);
            command->set_server_id(server_id);
            LaunchCommand(command);
        }
    }

    void MemcacheClientPool::PrefixMultiGet(evpp::EventLoop* caller_loop, std::vector<std::string>& keys, PrefixMultiGetCallback callback) {
        if (UNLIKELY(keys.size() <= 0)) {
            PrefixMultiGetResult result;
            caller_loop->RunInLoop(std::bind(callback, std::move(result)));
            return;
        }
        MultiModeVbucketConfig* vbconf = vbucket_config();
        const std::size_t size = keys.size();
        std::map<uint16_t, IdInfo> serverid_keys;
        std::vector<int> serverid_table(vbconf->server_list().size(), -1);

        uint16_t vbucket = 0;
        uint16_t server_id = 0;
        PrefixMultiKeyGetHandlerPtr handler = std::make_shared<MultiKeyHandler<PrefixMultiGetResult, PrefixMultiGetCallback> >(callback);
        auto& result = handler->get_result();
        std::size_t pos = 0;

        for (size_t i = 0; i < size; ++i) {
            auto& key = keys[i];
            GET_FILTER_KEY_POS(pos, key)
            vbucket = vbconf->GetVbucketByKey(key.c_str(), pos);
            server_id = vbconf->SelectServerFirstId(vbucket);
            if (serverid_table[server_id] == -1) {
                serverid_table[server_id] = vbconf->SelectServerId(vbucket, BAD_SERVER_ID);
            }
            server_id = serverid_table[server_id];
            auto& item = serverid_keys[server_id];
            item.keys.emplace_back(key);
            item.vbuckets.emplace_back(vbucket);
            result.emplace(std::move(key), std::make_shared<PrefixGetResult>());
        }
        handler->set_serverid_keys(serverid_keys);

        auto& serverid_keys_d = handler->get_serverid_keys();
        for (auto& it : serverid_keys_d) {
            server_id = it.first;
            CommandPtr command = std::make_shared<PrefixMultiGetCommand>(caller_loop, server_id, handler);
            command->set_server_id(server_id);
            LaunchCommand(command);
        }
    }

    void MemcacheClientPool::LaunchCommand(CommandPtr& command) {
        //const int thread = next_thread_++;
        auto loop = loop_pool_.GetNextLoopWithHash(rand());
        loop->RunInLoop(
            std::bind(&MemcacheClientPool::DoLaunchCommand, this, loop, command));
    }

    void MemcacheClientPool::DoLaunchCommand(evpp::EventLoop* loop, CommandPtr command) {
        MultiModeVbucketConfig* vbconf = vbucket_config();

        uint16_t server_id = command->server_id();
        if (UNLIKELY(!command->ShouldRetry())) { //重试 需要重新算serverid
            uint16_t vbucket = command->vbucket_id();
            server_id = vbconf->SelectServerId(vbucket, command->server_id());
            if (UNLIKELY(server_id == BAD_SERVER_ID)) {
                LOG_ERROR << "bad server id";
                command->OnError(ERR_CODE_DISCONNECT);
                return;
            }
        }

        std::string server_addr = vbconf->GetServerAddrById(server_id);
        MemcClientMap* client_map = GetMemcClientMap(loop);

        if (UNLIKELY(client_map == nullptr)) {
            command->OnError(ERR_CODE_DISCONNECT);
            LOG_INFO << "DoLaunchCommand thread pool empty";
            return;
        }

        auto it = client_map->find(server_addr);

        if (UNLIKELY(it == client_map->end())) {
            BuilderMemClient(loop, server_addr, *client_map, timeout_ms_);
            auto client = client_map->find(server_addr);
            client->second->PushWaitingCommand(command);
            return;
        }

        if (LIKELY(it->second->conn() && it->second->conn()->IsConnected())) {
            it->second->PushRunningCommand(command);
            command->Launch(it->second);
            return;
        }

        if (!it->second->conn() || it->second->conn()->status() == evpp::TCPConn::kConnecting) {
            it->second->PushWaitingCommand(command);
        } else {
            if (command->ShouldRetry()) {
                LOG_INFO << "OnClientConnection disconnect retry";
                command->set_id(0);
                command->set_server_id(command->server_id());
                LaunchCommand(command);
            } else {
                command->OnError(ERR_CODE_DISCONNECT);
            }
        }
    }
    }

