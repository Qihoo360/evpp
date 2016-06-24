#include "memcache_client_pool.h"
#include <thread>

namespace {

using namespace evmc;
static void OnTestSetDone(const std::string& key, int code) {
    LOG_INFO << "+++++++++++++ OnTestSetDone code=" << code << " " << key;
}
static void OnTestGetDone(const std::string& key, const GetResult& res) {
    LOG_INFO << "============= OnTestGetDone " << key << " code=" << res.code << " " << res.value;
}
static void OnTestRemoveDone(const std::string& key, int code) {
    LOG_INFO << "------------- OnTestRemoveDone code=" << code << " " << key;
}
static void OnTestMultiGetDone(const MultiGetResult& res) {
    LOG_INFO << ">>>>>>>>>>>>> OnTestMultiGetDone code=" << res.code;
    std::map<std::string, GetResult>::const_iterator it = res.get_result_map_.begin();
    for(; it != res.get_result_map_.end(); ++it) {
        LOG_INFO << ">>>>>>>>>>>>> OnTestMultiGetDone " << it->first << " " << it->second.code << " " << it->second.value;
    }
}

static EventLoopPtr g_loop;
static void StopLoop() {
    g_loop->Stop();
}

static void MyEventThread() {
    LOG_INFO << "EventLoop is running ...";
    g_loop = EventLoopPtr(new evpp::EventLoop);
    g_loop->Run();
}

}

int main() {
// TEST_UNIT(testMemcacheClient) {
    srand(time(NULL));
    std::thread th(MyEventThread);

    MemcacheClientPool mcp("./kill_storage_cluster.json", 32, 200);
    assert(mcp.Start());


    const static int MAX_KEY = 100;

    for(size_t i = 0; i < MAX_KEY; i += 2) {
        std::stringstream ss_key;
        ss_key << "test" << i;
        std::stringstream ss_value;
        ss_value << "test_value" << i;
        // usleep(10000);
        mcp.Set(g_loop, ss_key.str(), ss_value.str(), &OnTestSetDone);
    }

#if 1
    for(size_t i = 0; i < MAX_KEY; ++i) {
        std::stringstream ss;
        ss << "test" << i;
        mcp.Get(g_loop, ss.str().c_str(), &OnTestGetDone);
    }

    std::vector<std::string> mget_keys;
    for(size_t i = 0; i < MAX_KEY; ++i) {
        std::stringstream ss;
        ss << "test" << i;
        mget_keys.push_back(ss.str());
        if (mget_keys.size() >= 100) {
            break;
        }
    }
    mcp.MultiGet(g_loop, mget_keys, &OnTestMultiGetDone);

    for(size_t i = 0; i < MAX_KEY/2; ++i) {
        std::stringstream ss_key;
        ss_key << "test" << i;
        mcp.Remove(g_loop, ss_key.str().c_str(), &OnTestRemoveDone);
    }
#endif

    g_loop->RunAfter(1000.0, &StopLoop);
    th.join();
    return 0;
}


