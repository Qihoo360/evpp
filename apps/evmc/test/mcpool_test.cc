#include <evmc/memcache_client_serial.h>
#include <evmc/memcache_client_pool.h>
#include <evmc/vbucket_config.h>

#include <evpp/gettimeofday.h>

#include "../../../examples/winmain-inl.h"

#include <thread>

namespace {

using namespace evmc;
static struct timeval g_tv_begin;
static struct timeval g_tv_end;
static void OnTestSetDone(const std::string& key, int code) {
    LOG_INFO << "+++++++++++++ OnTestSetDone code=" << code << " " << key;
}
static void OnTestGetDone(const std::string& key, const GetResult& res) {
    LOG_INFO << "============= OnTestGetDone " << key << " code=" << res.code << " " << res.value;
}

static void OnTestPrefixDone(const std::string& prefix_key, const PrefixGetResultPtr res) {
    LOG_INFO << "************** OnTestPrefixGetDone prefix=" << prefix_key << " code=" << res->code;
    std::map<std::string, std::string>::const_iterator it = res->result_map_.begin();

    for (; it != res->result_map_.end(); ++it) {
        LOG_INFO << "<<<<<<<<<<<<<< OnTestPrefixGetDone " << it->first << " " << it->second;
    }
}

static void OnTestRemoveDone(const std::string& key, int code) {
    LOG_INFO << "------------- OnTestRemoveDone code=" << code << " " << key;
}
static void OnTestMultiGetDone(const MultiGetResult& res) {
    std::map<std::string, GetResult>::const_iterator it = res.begin();

    LOG_INFO << ">>>>>>>>>>>>> OnTestMultiGetDone";
    for (; it != res.end(); ++it) {
        LOG_INFO << "<<<<<<<<<< OnTestMultiGetDone " << it->first << " " << it->second.code << " " << it->second.value;
    }
}

static void OnTestPrefixMultiGetDone(const PrefixMultiGetResult& res) {
    gettimeofday(&g_tv_end, nullptr);
    LOG_INFO << "cost:" << (g_tv_end.tv_sec - g_tv_begin.tv_sec) * 1e6 + (g_tv_end.tv_usec - g_tv_end.tv_usec);
    LOG_INFO << ">>>>>>>>>>>>> OnTestPrefixMultiGetDone";
    auto it = res.begin();
    for (; it != res.end(); ++it) {
        OnTestPrefixDone(it->first, it->second);
    }
}

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

int main() {
// TEST_UNIT(testMemcacheClient) {
    //VbucketConfTest();
    //return 0;

#if 0
    g_loop = new evpp::EventLoop;
    std::thread th(MyEventThread);
    while (!g_loop->running()) {
        usleep(1000);
    }
    MemcacheClientPool mcp("./kill_storage_cluster.json", 4, 200);
    assert(mcp.Start());

    const static int MAX_KEY = 1000;

    for (size_t i = 0; i < MAX_KEY; ++i) {
        std::stringstream ss_key;
        ss_key << "test" << i;
        std::stringstream ss_value;
        ss_value << "test_value" << i;
        // mcp.Set(g_loop, ss_key.str(), ss_value.str(), &OnTestSetDone);
    }


    std::vector<std::string> mget_keys;
    for (size_t i = 1; i < 5/*MAX_KEY*/; ++i) {
        std::stringstream ss;
        //ss << "test" << i;
        //usleep(1000);
        ss << "test+" << i;
        std::string key(ss.str());
        //key.resize(12, 'T');
        // mcp.PrefixGet(g_loop, ss.str(), &OnTestPrefixDone);
        //mcp.Get(g_loop, key, &OnTestGetDone);
        mcp.Set(g_loop, key, key, &OnTestSetDone);
        mcp.Get(g_loop, key, &OnTestGetDone);
        mget_keys.push_back(key);
        //mcp.Get(g_loop, key, &OnTestGetDone);
        //mcp.Remove(g_loop, key, &OnTestRemoveDone);
        //mcp.Get(g_loop, key, &OnTestGetDone);
        //mcp.Set(g_loop, key, key, &OnTestSetDone);
    }
    mcp.PrefixGet(g_loop, "test", &OnTestPrefixDone);
    mcp.MultiGet(g_loop, mget_keys, &OnTestMultiGetDone);
    mget_keys.clear();
    std::stringstream ps;
    ps << "test";
    mget_keys.push_back(ps.str());
    mget_keys.push_back(ps.str());
    mget_keys.push_back(ps.str());

    mcp.PrefixMultiGet(g_loop, mget_keys, &OnTestPrefixMultiGetDone);

    std::stringstream ss;
    int count = 0;
    for (size_t i = 0; i < 1/*MAX_KEY*/; ++i) {
        mget_keys.clear();
        for (size_t j = 1; j < 2; j++) {
            ss.str("");
            ss << j;
            mget_keys.push_back(ss.str());
        }
        count++;
        //mcp.MultiGet(g_loop, mget_keys, &OnTestMultiGetDone);
        gettimeofday(&g_tv_begin, nullptr);
        //mcp.PrefixMultiGet(g_loop, mget_keys, &OnTestPrefixMultiGetDone);
    }
    LOG_INFO << "count value:" << count;
// mcp.PrefixMultiGet(g_loop, mget_keys, &OnTestPrefixMultiGetDone);
    //mcp.MultiGet(g_loop, mget_keys, &OnTestMultiGetDone);

    for (size_t i = 0; i < MAX_KEY; ++i) {
        std::stringstream ss_key;
        ss_key << "test" << i;
        usleep(1000);
//      mcp.Remove(g_loop, ss_key.str().c_str(), &OnTestRemoveDone);
    }
    g_loop->RunAfter(10.0, &StopLoop);
    mcp.Stop(true);
    th.join();


#else

    g_loop = new evpp::EventLoop;
    std::thread th(MyEventThread);
    while (!g_loop->IsRunning()) {
        usleep(1000);
    }
    MemcacheClientSerial mcp("10.102.16.25:20099", 200);
    assert(mcp.Start(g_loop));
    usleep(2 * 1000 * 1000);
    std::string key("test");
    std::string value("test1");
    mcp.Set(key, value, &OnTestSetDone);
    std::string monkey = "monkey";
    mcp.Get(monkey, &OnTestGetDone);
    std::string key1("dog");
    std::string value1("dog1");
    mcp.Set(key1, value1, &OnTestSetDone);
    std::string key2("cat");
    std::string value2("cat1");
    mcp.Set(key2, value2, &OnTestSetDone);
    std::string key3("cat1");
    std::string value3("cat2");
    mcp.Set(key3, value3, &OnTestSetDone);
    mcp.Get(key, &OnTestGetDone);
    mcp.Remove(key, &OnTestRemoveDone);
    std::vector<std::string> mget_keys;
    mget_keys.push_back("monkey");
    mget_keys.push_back("test");
    mget_keys.push_back("dog");
    mget_keys.push_back("cat");
    mget_keys.push_back("cat1");
    while (1) {
        mcp.MultiGet(mget_keys, &OnTestMultiGetDone);
        usleep(100000);
    }

    g_loop->RunAfter(10.0, &StopLoop);
    th.join();
    //mcp.Stop(true);
    mcp.Stop();
#endif
    return 0;
}


