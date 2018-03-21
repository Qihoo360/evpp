#include "memcache_client_pool.h"
#include "folly/MoveWrapper.h"

#include "vbucket_config.h"
#include "evpp/event_loop_thread_pool.h"
#include "likely.h"
#include "command.h"

namespace evmc {

#define GET_FILTER_KEY_POS(name, key) \
    name = key.find(key_filter_[0]); \
    if (name == std::string::npos) { \
        name = key.size(); \
    }

MemcacheClientPool::~MemcacheClientPool() {
    if (loop_pool_.IsRunning()) {
        Stop(true);
    }
}

void MemcacheClientPool::Stop(bool wait_thread_exit) {
    loop_pool_.Stop(wait_thread_exit);
}

void MemcacheClientPool::BuilderMemClient(evpp::EventLoop* loop, std::string& server, std::map<std::string, MemcacheClientPtr>& client_map, const int timeout_ms) {
    evpp::TCPClient* tcp_client = new evpp::TCPClient(loop, server, "evmc");
    MemcacheClientPtr memc_client = std::make_shared<MemcacheClient>(loop, tcp_client, this, timeout_ms);

    LOG_INFO << "Start new tcp_client=" << tcp_client << " server=" << server << " timeout=" << timeout_ms;

    tcp_client->SetConnectionCallback(std::bind(&MemcacheClient::OnClientConnection, memc_client,
                                      std::placeholders::_1));
    tcp_client->SetMessageCallback(std::bind(&MemcacheClient::OnResponseData, memc_client,
                                   std::placeholders::_1, std::placeholders::_2));
    tcp_client->Connect();
    client_map.emplace(server, memc_client);
}


bool MemcacheClientPool::Start() {
    bool ok = vbucket_config_->Load(vbucket_conf_file_.c_str());
    if (UNLIKELY(!ok)) {
        LOG_ERROR << "vbucket load failed";
        return false;
    }

    ok = loop_pool_.Start(true);

    if (UNLIKELY(!ok)) {
        LOG_ERROR << "loop pool start failed";
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

template<class CmdType, class RetType>
folly::Future<RetType> MemcacheClientPool::LaunchOneKeyCommand(const std::string& key, const std::string& value) {
    std::size_t pos = 0;
    GET_FILTER_KEY_POS(pos, key);
    const uint16_t vbucket = vbucket_config()->GetVbucketByKey(key.c_str(), pos);
    std::vector<std::string> send_data = {key};
    if (!value.empty()) {
        send_data.emplace_back(value);
    }
    std::vector<int> vbucket_ids = {vbucket};
    folly::Promise<RetType> promise;
    folly::Future<RetType> fut = promise.getFuture();
    std::vector<uint16_t> server_ids;
    auto server_id = vbucket_config()->SelectServerId(vbucket, server_ids);
    CommandPtr command = std::make_shared<CmdType>(server_id, send_data, vbucket_ids, promise);
    LaunchCommand(command);
    return std::move(fut);
}

intFuture MemcacheClientPool::Set(const std::string& key, const std::string& value) {
    return std::move(LaunchOneKeyCommand<SetCommand, intExpected>(key, value));
}

intFuture MemcacheClientPool::Remove(const std::string& key) {
    return std::move(LaunchOneKeyCommand<RemoveCommand, intExpected>(key, ""));
}

str2mapFuture MemcacheClientPool::PrefixGet(const std::string& key) {
    return std::move(LaunchOneKeyCommand<PrefixGetCommand, str2mapExpected>(key, ""));
}

stringFuture MemcacheClientPool::Get(const std::string& key) {
    return std::move(LaunchOneKeyCommand<GetCommand, stringExpected>(key, ""));
}

#define WaitInLoop() \
    caller_loop->RunInLoop([f = folly::makeMoveWrapper(std::move(future)), &key, callback, caller_loop]() mutable {\
        auto & fut = f->waitVia(caller_loop); \
        auto expected = std::move(fut.value());\
        if (expected.hasValue()) {\
            callback(key, expected.value()); \
            return; \
        } else { \
            callback(key, expected.error()); \
        } \
        return; \
    }); \

void MemcacheClientPool::Set(evpp::EventLoop* caller_loop, const std::string& key, const std::string& value, SetCallback& callback) {
    auto future = std::move(Set(key, value));
    WaitInLoop();
}

void MemcacheClientPool::Remove(evpp::EventLoop* caller_loop, const std::string& key, RemoveCallback& callback) {
    auto future = std::move(Remove(key));
    WaitInLoop();
}

void MemcacheClientPool::Get(evpp::EventLoop* caller_loop, const std::string& key, GetCallback& callback) {
    auto future = std::move(Get(key));
    caller_loop->RunInLoop([f = folly::makeMoveWrapper(std::move(future)), key, callback, caller_loop]() mutable {
        auto& fut = f->waitVia(caller_loop);
        auto expected = std::move(fut.value());
        GetResult gr;
        if (expected.hasValue()) {
            gr.set(EVMC_SUCCESS, std::move(expected.value()));
            callback(key, gr);
            return;
        } else {
            gr.error(expected.error());
            callback(key, gr);
        }
        return;
    });
}

void MemcacheClientPool::PrefixGet(evpp::EventLoop* caller_loop, const std::string& key, PrefixGetCallback& callback) {
    auto future = PrefixGet(key);
    caller_loop->RunInLoop([f = folly::makeMoveWrapper(std::move(future)), key, callback, caller_loop]() mutable {
        auto& fut = f->waitVia(caller_loop);
        auto expected = std::move(fut.value());
        PrefixGetResultPtr pgr = std::make_shared<PrefixGetResult>();
        if (expected.hasValue()) {
            auto pmap = std::move(expected.value());
            if (!pmap.empty() && !pmap.begin()->second.empty()) {
                pgr->set(EVMC_SUCCESS, std::move(pmap.begin()->second));
            } else {
                pgr->error(EVMC_KEY_ENOENT);
            }
            callback(key, pgr);
            return;
        } else {
            pgr->error((int)expected.error());
            callback(key, pgr);
        }
        return;
    });
}

void MemcacheClientPool::RunBackGround(const std::function<void(void)>& fun) {
    auto loop = loop_pool_.GetNextLoopWithHash(rand());
    loop->RunInLoop(fun);
}

void MemcacheClientPool::PrefixMultiGet(evpp::EventLoop* caller_loop, std::vector<std::string>& keys, PrefixMultiGetCallback& callback) {
    auto future = PrefixMultiGet(keys);
    PrefixMultiGetResult pmgresult;
    PrefixGetResultPtr pgr;
    for (auto& key : keys) {
        pgr = std::make_shared<PrefixGetResult>();
        pmgresult.emplace(key, pgr);
    }
    caller_loop->RunInLoop([f = folly::makeMoveWrapper(std::move(future)), pmgr = std::move(pmgresult), callback, caller_loop]() mutable {
        auto& fut = f->waitVia(caller_loop);
        auto expected = std::move(fut.value());
        auto it = pmgr.begin();
        if (expected.hasValue()) {
            auto pmmap = std::move(expected.value());
            for (auto& kv : pmmap) {
                it = pmgr.find(kv.first);
                if (it == pmgr.end() || kv.second.empty()) {
                    continue;
                }
                it->second->set(EVMC_SUCCESS, std::move(kv.second));
            }
            callback(pmgr);
            return;
        } else {
            for (auto& kv : pmgr) {
                kv.second->error((int)expected.error());
                callback(pmgr);
            }
        }
        return;
    });
}


void MemcacheClientPool::MultiGet(evpp::EventLoop* caller_loop, const std::vector<std::string>& keys, MultiGetCallback& callback) {
    auto future = MultiGet(keys);
    MultiGetResult mgresult;
    for (auto& key : keys) {
        mgresult.emplace(key, GetResult());
    }
    caller_loop->RunInLoop([f = folly::makeMoveWrapper(std::move(future)), mgr = std::move(mgresult), callback, caller_loop]() mutable {
        auto& fut = f->waitVia(caller_loop);
        auto expected = std::move(fut.value());
        if (expected.hasValue()) {
            std::map<std::string, std::string> k_v(std::move(expected.value()));
            auto k_v_iter = k_v.end();
            for (auto& it : mgr) {
                k_v_iter = k_v.find(it.first);
                if (k_v_iter != k_v.end()) {
                    it.second.set(EVMC_SUCCESS, std::move(k_v_iter->second));
                }
            }
        } else {
            auto error = expected.error();
            for (auto& it : mgr) {
                it.second.error(error);
            }
        }
        callback(mgr);
    });
}

template <class CmdType, class RetType>
void MemcacheClientPool::LaunchMultiKeyCommand(const std::vector<std::string>& keys, std::vector<folly::Future<RetType>>& futs) {
    const std::size_t size = keys.size();

    MultiModeVbucketConfig* vbconf = vbucket_config();
    uint16_t vbucket = 0;
    uint16_t first_server_id = 0;
    uint16_t server_id = 0;
    std::size_t pos = 0;
    std::map<int, CommandPtr> serverid_cmd;
    std::vector<uint16_t> server_ids;
    for (size_t i = 0; i < size; ++i) {
        auto& key = keys[i];
        GET_FILTER_KEY_POS(pos, key)
        vbucket = vbconf->GetVbucketByKey(key.c_str(), pos);
        first_server_id = vbconf->SelectServerFirstId(vbucket);

        //同一个server的指令聚合
        if (serverid_cmd.find(first_server_id) == serverid_cmd.end()) {
            server_id = vbconf->SelectServerId(vbucket, server_ids);
            assert(server_id != BAD_SERVER_ID);
            folly::Promise<RetType> p;
            futs.emplace_back(p.getFuture());
            std::vector<std::string> ks = {key};
            std::vector<int> vbucket_ids = {vbucket};
            auto cmd = std::make_shared<CmdType>(server_id, ks, vbucket_ids, p);
            serverid_cmd.emplace(server_id, std::move(cmd));
        } else {
            auto& cmd = serverid_cmd[first_server_id];
            cmd->send_data().emplace_back(key);
            cmd->vbucketids().emplace_back(vbucket);
        }
    }

    for (auto& it : serverid_cmd) {
        LaunchCommand(it.second);
    }
}

template <class RetType>
folly::Future<RetType> MemcacheClientPool::CollectAllFuture(std::vector<folly::Future<RetType>>& futs) {
    auto fut = folly::collectAll(futs).then([](const std::vector<folly::Try<RetType>>& tries) ->folly::Future<RetType> {
        folly::ExpectedValueType<RetType> ret_data;
        for (auto& t : tries) {
            if (!t.hasValue()) {
                return folly::makeUnexpected(EVMC_EXCEPTION);
            }
            auto expect = t.value();
            if (expect.hasValue()) {
                if (ret_data.empty()) {
                    ret_data.swap(expect.value());
                } else {
                    for (auto& k_v : expect.value()) {
                        ret_data.emplace(std::move(k_v));
                    }
                }
                continue;
            }
            return folly::makeUnexpected(expect.error());
        }
        return folly::makeExpected<EvmcCode>(std::move(ret_data));
    }
                                           );
    return std::move(fut);
}

str2mapFuture MemcacheClientPool::PrefixMultiGet(const std::vector<std::string>& keys)  {
    if (UNLIKELY(keys.size() <= 0)) {
        str2map ret_data;
        return folly::makeFuture<str2mapExpected>(folly::makeExpected<EvmcCode>(std::move(ret_data)));
    }
    std::vector<str2mapFuture> futs;
    LaunchMultiKeyCommand<PrefixGetCommand>(keys, futs);
    return CollectAllFuture(futs);
}


str2strFuture MemcacheClientPool::MultiGet(const std::vector<std::string>& keys) {
    if (UNLIKELY(keys.size() <= 0)) {
        std::map<std::string, std::string> ret_data;
        return folly::makeFuture<str2strExpected>(folly::makeExpected<EvmcCode>(std::move(ret_data)));
    }
    std::vector<str2strFuture> futs;
    LaunchMultiKeyCommand<MultiGetCommand>(keys, futs);
    return CollectAllFuture(futs);
}

void MemcacheClientPool::LaunchCommand(CommandPtr& command) {
    auto loop = loop_pool_.GetNextLoopWithHash(rand());
    loop->RunInLoop(std::bind(&MemcacheClientPool::DoLaunchCommand, this, loop, command));
}

void MemcacheClientPool::ReLaunchCommand(evpp::EventLoop* loop, CommandPtr& command) {
    MultiModeVbucketConfig* vbconf = vbucket_config();
    auto server_id = vbconf->SelectServerId(command->vbucketids()[0], command->server_ids());
    if (server_id == BAD_SERVER_ID) {
        command->error(EVMC_ALLDOWN);
        return;
    }
    command->set_server_id(server_id);
    DoLaunchCommand(loop, command);
}

void MemcacheClientPool::DoLaunchCommand(evpp::EventLoop* loop, CommandPtr command) {
    MultiModeVbucketConfig* vbconf = vbucket_config();

    uint16_t server_id = *command->server_ids().rbegin();

    std::string server_addr = vbconf->GetServerAddrById(server_id);
    MemcClientMap* client_map = GetMemcClientMap(loop);
    assert(client_map != nullptr);

    auto it = client_map->find(server_addr);
    assert(it != client_map->end());
    it->second->Launch(command);
}
}

