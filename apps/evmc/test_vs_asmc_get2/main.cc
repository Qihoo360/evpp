#include "cluster_case.h"
#include <event2/event.h>
#include <gflags/gflags.h>
#include "evmc/memcache_client_pool.h"
#include "evmc/memcache_client_single.h"

//static const char* moxi_host = "182.118.21.171";
//static uint16_t moxi_port = 11221;
//static const char* conf = "http://182.118.21.171:8360/storage_cluster_config_failover";


//static const char* conf = "http://182.118.21.171:8360/storage_cluster_config_cleanpage";

DEFINE_int32(thread_nums, 10, "thread pool size");
DEFINE_int32(timeout_ms, 5000, "read or write timeout");
DEFINE_int32(key_size, 12, "memecached test key size");
DEFINE_int32(packet_nums, 1000, "send total request nums to memcached");
DEFINE_int32(qps, 1000, "send request packets per second"); 
DEFINE_int32(test_type, 6, "0:set 1:get 2:MultiGet 3:remove"); 
DEFINE_int32(multiget_nums, 5, "multiget key nums per transaction"); 
DEFINE_int32(prefixmultiget_nums, 5, "prefixmultiget key nums per transaction"); 
DEFINE_int32(worker_threads, 1, "work thread nums"); 
static std::vector<std::vector<std::string> > g_data;
static Timer g_timer[50];
static int g_count[50];
static evmc::MemcacheClientPool * g_mpc = NULL;
static evmc::MemcacheClientSingle * g_mpc_s[50];
static evpp::EventLoop * g_loop[50];


static void RunTask(evpp::EventLoop * event_loop, const int i);

static void ThreadFun1(int i) {
	evpp::EventLoop * event_loop = new evpp::EventLoop();
	assert(NULL != event_loop);
	g_loop[i] = event_loop;
	event_loop->Run();
}

static void ThreadFun(int i) {
	evpp::EventLoop * event_loop = new evpp::EventLoop();
	assert(NULL != event_loop);
	std::thread th(ThreadFun1, i);
	th.detach();
	while(g_loop[i] == NULL) {
		usleep(10000);
	}
	event_loop->RunAfter(0.01, std::bind(RunTask, event_loop, i));
	event_loop->Run();
}

std::string NumberToString(int number) {
	 std::ostringstream oss;
	  oss << number;
	  return oss.str();
}


bool MakeData(std::string& key, const int32_t index,  const std::size_t size) {
	key.clear();
    key.assign(NumberToString(index));
    if (key.size() < size) {
       key.resize(size, 'T');
    }
    return true;
}


void ClusterMultiGetCallback(int i, const  evmc::MultiGetResult & m) {
    const std::map<std::string, evmc::GetResult>& rts =  m.get_result_map_;
    std::map<std::string, evmc::GetResult>::const_iterator it = rts.begin();
    std::map<std::string, evmc::GetResult>::const_iterator end = rts.end();
    for ( ; it != end; ++it) {
		const evmc::GetResult& rt = it->second;
		if (0 != rt.code || 0 != (it->first).compare(rt.value)) {
			LOG(WARNING) << "MultiGet ret error, rtcode=" << rt.code << " key=" << it->first << ",value=" << rt.value;
			continue;
		} else {
			//LOG(INFO) << "MultiGet ret success, rtcode=" << rt.code << " key=" << it->first << ",value=" << rt.value;
		}
	}
	int count = ++g_count[i];
	if (count  >= FLAGS_packet_nums) {
		Timer &t = g_timer[i];
		t.End();
		LOG(WARNING) << pthread_self() << ":cost(us)--" << t.Elapsedus(); 
		exit(0);
	}
}

void ClusterMultiGet2Callback(int i, const  evmc::MultiGetMapResultPtr & m, int res_code) {
	for (auto it = m->begin(); it != m->end(); ++it) {
		auto rt = it->second;
		if (0 != rt.code || 0 != (it->first).compare(rt.value)) {
			LOG(WARNING) << "MultiGet ret error, rtcode=" << rt.code << " key=" << it->first << ",value=" << rt.value;
			continue;
		} else {
			//LOG(WARNING) << "MultiGet ret success, rtcode=" << rt.code << " key=" << it->first << ",value=" << rt.value;
		}
	}
	int count = ++g_count[i];
	if (count  >= FLAGS_packet_nums) {
		Timer &t = g_timer[i];
		t.End();
		LOG(WARNING) << pthread_self() << ":cost(us)--" << t.Elapsedus(); 
		//exit(0);
		return;
	}
   
	g_mpc_s[i]->MultiGet2(g_loop[i], g_data[i], std::bind(&ClusterMultiGet2Callback, i, std::placeholders::_1, std::placeholders::_2));
}

void RunTask(evpp::EventLoop * event_loop, const int i) {
	g_timer[i].Begin();
	g_count[i] = 0;
	//assert(g_loop[i] != NULL);
	for (int j = 0; j < FLAGS_packet_nums; ++j) {
		switch (FLAGS_test_type) {
			case 2:
				g_mpc->MultiGet(g_loop[i], g_data[j], std::bind(&ClusterMultiGetCallback, i, std::placeholders::_1));
				break;
			case 6:
				g_mpc->MultiGet2(g_loop[i], g_data[j], std::bind(&ClusterMultiGet2Callback, 
							i, std::placeholders::_1, std::placeholders::_2));
				break;
			default:
				break;
		}
	}
		//Timer &t = g_timer[i];
		//t.End();
		//LOG(WARNING) << pthread_self() << ":cost(us)--" << t.Elapsedus(); 
}

void ThreadFun11(int i) {
		g_mpc_s[i] = new evmc::MemcacheClientSingle("123.125.160.25:20099", FLAGS_thread_nums, FLAGS_timeout_ms);
		g_loop[i] = new evpp::EventLoop();
		bool rt = g_mpc_s[i]->Start(g_loop[i]);
		assert(rt == true);
		g_timer[i].Begin();
		g_mpc_s[i]->MultiGet2(g_loop[i], g_data[i], std::bind(&ClusterMultiGet2Callback, i, std::placeholders::_1, std::placeholders::_2));
		g_loop[i]->Run();
}

int main(int argc, char *argv[]) {
	::gflags::ParseCommandLineFlags(&argc, &argv, true);  
	//g_mpc = new evmc::MemcacheClientPool("./kill_storage_cluster.json", FLAGS_thread_nums, FLAGS_timeout_ms);

	int k = 0;
	std::string key;
	for (int i = 0; i < FLAGS_packet_nums; ++i) {
		g_data.resize(g_data.size() + 1);
		auto & keys = g_data.back();
		keys.clear();
		k = i;
		for (int num = 0; num < FLAGS_multiget_nums; ++num) {
			MakeData(key, k++, FLAGS_key_size);
			keys.push_back(key);
		}
	}
//	for (int i = 0; i < Flags_worker_nums; i++) {
	for (int i = 0; i < 50 && i < FLAGS_worker_threads; ++i) {
		g_loop[i] = NULL;
		std::thread th(ThreadFun11, i);
		th.detach();
	}

	//for (int i = 0; i < 50 && i < FLAGS_worker_threads; ++i) {
		//g_loop[i] = NULL;
		//std::thread th(ThreadFun, i);
		//th.detach();
	//}
	/*int i = 10;
	while (i-- > 0) {
		sleep(1);
	}*/
	//delete g_mpc;
    //std::thread th(MyEventThread);
	//th.detach();

    return 0;
}

