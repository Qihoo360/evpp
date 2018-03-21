#include <evmc/memcache_client_pool.h>
#include <evmc/vbucket_config.h>

#include <evpp/gettimeofday.h>

#include "../../../examples/winmain-inl.h"
#include "folly/synchronization/Baton.h"

#include <thread>

namespace {

using namespace evmc;

static evpp::EventLoop* g_loop;
static void StopLoop() {
    LOG_INFO << "EventLoop is stopping ...";
    g_loop->Stop();
}

static void MyEventThread() {
    LOG_INFO << "EventLoop is running ...";
    g_loop->Run();
}

static const char* keys[] = {
    "hello",
    "doctor",
    "name",
    "continue",
    "yesterday",
    "tomorrow",
    "another key"
};

void VbucketConfTest() {
    VbucketConfig* conf = new VbucketConfig();

    if (!conf->Load("./test_kill_storage_cluster.json")) {
        LOG_ERROR << "VbucketConfTest load error";
        return;
    }

    for (size_t i = 0; i < sizeof(keys) / sizeof(keys[0]); ++i) {
        uint16_t vbucket = conf->GetVbucketByKey(keys[i], strlen(keys[i]));
        LOG_INFO << "VbucketConfTest key=" << keys[i] << " vbucket=" << vbucket;
    }
}
}

void TestSet(MemcacheClientPool& mcp) {
    std::string key("testt+0");
    auto fut = mcp.Set(key, key).wait();
    auto result = fut.value();
    assert(result.hasValue());
    int ret = result.value();
    LOG_ERROR << "ret:" << ret;
    folly::Baton<> bat;
    SetCallback scb([baton = std::ref(bat)](const std::string & k, int code) {
        LOG_ERROR << "set key:" << k << ", code=" << code;
        baton.get().post();
    });
    mcp.Set(g_loop, key, key, scb);
    bat.wait();
}

void TestRemove(MemcacheClientPool& mcp) {
    std::string key("testt+0");
    auto fut = mcp.Remove(key).wait();
    auto result = fut.value();
    assert(result.hasValue());
    int ret = result.value();
    LOG_ERROR << "ret:" << ret;
    folly::Baton<> bat;
    RemoveCallback rcb([baton = std::ref(bat)](const std::string & k, int code) {
        LOG_ERROR << "remove key:" << k << ", code=" << code;
        baton.get().post();
    });
    mcp.Remove(g_loop, key, rcb);
    bat.wait();
}

void TestGet(MemcacheClientPool& mcp) {
    std::string key("testtt+0");
    auto fut = mcp.Get(key).wait();
    auto result = fut.value();
    if (result.hasValue()) {
        auto ret = result.value();
        LOG_ERROR << "get value:" << ret;
    } else {
        LOG_ERROR << "get error:" << result.error();
    }
    folly::Baton<> bat;
    GetCallback gcb([baton = std::ref(bat)](const std::string & k, const GetResult & r) {
        LOG_ERROR << "get key:" << k << ", code=" << r.code << ",value=" << r.value;
        baton.get().post();
    });
    mcp.Get(g_loop, key, gcb);
    bat.wait();
}

void TestMultiGet(MemcacheClientPool& mcp) {
    std::vector<std::string> mget_keys;
    int num = 1;
    while (num-- > 0) {
        mget_keys.clear();
        for (size_t i = 0; i < 5; ++i) {
            std::stringstream ss;
            ss << "test+" << i;
            std::string key(ss.str());
            mget_keys.push_back(key);
            mget_keys.push_back(key);
        }
        auto ret = mcp.MultiGet(mget_keys).wait();
        assert(ret.hasValue());
        auto result = ret.value();
        if (result.hasValue()) {
            for (auto& it : result.value()) {
                LOG_ERROR << "key:" << it.first << ", value=" << it.second;
            }
        } else {
            LOG_ERROR << "error:" << result.error();
        }
        MultiGetCallback callback([](const MultiGetResult & mgr) {
            for (auto& it : mgr) {
                LOG_ERROR << "key:" << it.first << ", code=" << it.second.code << ", value" << it.second.value;
            }
        });
        mcp.MultiGet(g_loop, mget_keys, callback);
    }
    sleep(1);
}


void Printstr2mapFuture(str2mapFuture& future) {
    auto result = std::move(future.value());
    if (result.hasValue()) {
        for (auto& kv : result.value()) {
            LOG_ERROR << "prefix key=" << kv.first;
            for (auto& it : kv.second) {
                LOG_ERROR << "Get key=" << it.first << ", value=" << it.second;
            }
        }
    } else {
        LOG_ERROR << "PrefixGet error=" << result.error();
    }
}

void TestPrefixGet(MemcacheClientPool& mcp) {
    std::string key("test");
    auto future = mcp.PrefixGet(key).wait();
    Printstr2mapFuture(future);
    folly::Baton<> bat;
    PrefixGetCallback pgcb([baton = std::ref(bat)](const std::string & k, const PrefixGetResultPtr result) {
        LOG_ERROR << "prefix key=" << k << ",code=" << result->code;
        for (auto& kv : result->result_map) {
            LOG_ERROR << "get key=" << kv.first << ",value=" << kv.second;
        }
        baton.get().post();
    });
    mcp.PrefixGet(g_loop, key, pgcb);
    bat.wait();
}

void TestMultiPrefixGet(MemcacheClientPool& mcp) {
    std::vector<std::string> keys = {"test", "1test"};
    auto future = mcp.PrefixMultiGet(keys).wait();
    Printstr2mapFuture(future);
    folly::Baton<> bat;
    PrefixMultiGetCallback pmgcb([baton = std::ref(bat)](const PrefixMultiGetResult & result) {
        for (auto& kv : result) {
            LOG_ERROR << "prefixmulti key=" << kv.first << ",code=" << kv.second->code;
            for (auto& it : kv.second->result_map) {
                LOG_ERROR << "get key=" << it.first << ",value=" << it.second;
            }
        }
        baton.get().post();
    });
    mcp.PrefixMultiGet(g_loop, keys, pmgcb);
    bat.wait();
}

int main() {
    g_loop = new evpp::EventLoop;
    std::thread th(MyEventThread);
    g_loop->WaitUntilRunning();
    MemcacheClientPool mcp("./kill_storage_cluster.json", 4, 200);
    bool ok = mcp.Start();
    if (!ok) {
        LOG_ERROR << "init failed";
        return -1;
    }
    assert(ok);

    TestSet(mcp);
    TestGet(mcp);
    TestRemove(mcp);
    TestMultiGet(mcp);
    TestPrefixGet(mcp);
    TestMultiPrefixGet(mcp);



    g_loop->RunAfter(1, &StopLoop);
    mcp.Stop(true);
    th.join();
    delete g_loop;
    return 0;
}


