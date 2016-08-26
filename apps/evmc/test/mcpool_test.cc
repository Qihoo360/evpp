#include <evmc/exp.h>
#include <evmc/memcache_client_pool.h>
#include <evmc/vbucket_config.h>

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
    std::map<std::string, std::string>::const_iterator it = res->get_result_map_.begin();

    for (; it != res->get_result_map_.end(); ++it) {
        LOG_INFO << "<<<<<<<<<<<<<< OnTestPrefixGetDone " << it->first << " " << it->second;
	}
}

static void OnTestRemoveDone(const std::string& key, int code) {
    LOG_INFO << "------------- OnTestRemoveDone code=" << code << " " << key;
}
static void OnTestMultiGetDone(const MultiGetResult& res) {
    LOG_INFO << ">>>>>>>>>>>>> OnTestMultiGetDone code=" << res.code;
    std::map<std::string, GetResult>::const_iterator it = res.get_result_map_.begin();

    for (; it != res.get_result_map_.end(); ++it) {
        LOG_INFO << "<<<<<<<<<< OnTestMultiGetDone " << it->first << " " << it->second.code << " " << it->second.value;
    }
}

static void OnTestPrefixMultiGetDone(PrefixMultiGetResultPtr res) {
	  gettimeofday(&g_tv_end, NULL);
	  LOG_INFO << "cost:" << (g_tv_end.tv_sec - g_tv_begin.tv_sec) * 1e6 + (g_tv_end.tv_usec - g_tv_end.tv_usec); 
    LOG_INFO << ">>>>>>>>>>>>> OnTestPrefixMultiGetDone code=" << res->code;
	auto it = res->get_result_map_.begin();
	for (; it != res->get_result_map_.end(); ++it) {
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
    g_loop = new evpp::EventLoop;
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

    srand(time(NULL));
    std::thread th(MyEventThread);

    MemcacheClientPool mcp("./kill_storage_cluster.json", 4, 200);
    assert(mcp.Start());

    const static int MAX_KEY = 1000;

    for (size_t i = 0; i < MAX_KEY; ++i) {
        std::stringstream ss_key;
        ss_key << "test" << i;
        std::stringstream ss_value;
        ss_value << "test_value" << i;
        usleep(1000);
       // mcp.Set(g_loop, ss_key.str(), ss_value.str(), &OnTestSetDone);
    }

#if 1

   for (size_t i = 1; i < 2/*MAX_KEY*/; ++i) {
       std::stringstream ss;
       //ss << "test" << i;
       //usleep(1000);
	   ss << 0;
	   std::string key(ss.str());
	   //key.resize(12, 'T');
	   //mcp.PrefixGet(g_loop, ss.str(), &OnTestPrefixDone);
	  //mcp.Set(g_loop, key, key, &OnTestSetDone);
      //mcp.Get(g_loop, key, &OnTestGetDone);
   }

  std::vector<std::string> mget_keys;
  std::stringstream ss;
  int count = 0;
  for(size_t i = 0; i < 1/*MAX_KEY*/; ++i) {
	  mget_keys.clear();
	  for (size_t j = 1; j < 2; j++) {
		  ss.str("");
		  ss << j;
		  mget_keys.push_back(ss.str());
	  }
	  count++;
	  //mcp.MultiGet(g_loop, mget_keys, &OnTestMultiGetDone);
	  gettimeofday(&g_tv_begin, NULL);
	  mcp.PrefixMultiGet(g_loop, mget_keys, &OnTestPrefixMultiGetDone);
  }
  LOG_INFO << "count value:" << count;
 // mcp.PrefixMultiGet(g_loop, mget_keys, &OnTestPrefixMultiGetDone);
  //mcp.MultiGet(g_loop, mget_keys, &OnTestMultiGetDone);

    for (size_t i = 0; i < MAX_KEY; ++i) {
        std::stringstream ss_key;
        ss_key << "test" << i;
        usleep(1000);
//		mcp.Remove(g_loop, ss_key.str().c_str(), &OnTestRemoveDone);
    }

#endif

	usleep(2*1000*1000);
    g_loop->RunAfter(1.0, &StopLoop);
    th.join();
    mcp.Stop(true);
    return 0;
}


