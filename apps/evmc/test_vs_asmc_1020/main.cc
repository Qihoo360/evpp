#include <event2/event.h>
#include <gflags/gflags.h>
#include "evmc/exp.h"
#include "evmc/memcache_client_pool.h"

DEFINE_string(host, "./kill_storage_cluster.json", "host");
DEFINE_string(key, "test", "query key");
static evpp::EventLoop* g_loop = NULL;
static int32_t g_running = false;
static void MyEventThread() {
	g_running = true;
	g_running = false;
	std::cout << "exit" << std::endl;
	
}

bool ConvertFromBinary(const std::string &data)
{
	    const char *p = data.c_str();
		uint32_t tmp = 0; 
		const int len = data.size();
		tmp = (p[0] >> 6) & 0x00000001;
		std::cout << "src:" << tmp << std::endl;
		tmp = (p[0] >> 5) & 0x00000001;
		std::cout << "upload:" << tmp << std::endl;
		tmp = (p[0] >> 4) & 0x00000001;
		std::cout << "attr_upload:" << tmp << std::endl;
		tmp = p[0] & 0x0000000F;
		std::cout << "flag:" << tmp << std::endl;
		tmp = p[1] & 0x000000FF ; 
		std::cout << "level:" << tmp << std::endl;
		tmp = p[2] & 0x000000FF;
		std::cout << "sub_level:" << tmp << std::endl;
		if(len > 3) {
			uint8_t map = p[3];
			int key_count = 0;
			for(int i = 0; i < 8; ++i, ++key_count) {
				tmp = 0;
				if((map >> (7-i) & 0x01) == 1) {
					tmp |= ((uint32_t) p[4 + 4 * key_count] << 24) & 0xFF000000;
					tmp |= ((uint32_t) p[5 + 4 * key_count] << 16) & 0x00FF0000;
					tmp |= ((uint32_t) p[6 + 4 * key_count] << 8) & 0x0000FF00;
					tmp |= ((uint32_t) p[7 + 4 * key_count] << 0) & 0x000000FF;
					if(i == 0) {
						std::cout << "file_desc:" << tmp << std::endl;
						continue;
					}
					if(i == 1) {
						std::cout << "softname:" << tmp << std::endl;
						continue;
					}
					if (i == 2) {
						std::cout << "describe:" << tmp << std::endl;
						continue;
					}
					if (i == 3) {
						std::cout << "malware:" << tmp << std::endl;
						continue;
					}
					if (i == 4) {
						std::cout << "class:" << tmp << std::endl;
						continue;
					}
					if (i == 6) {
						std::cout << "extinfo:" << tmp << std::endl;
						continue;
					}
				}
			}
		}
}


void ClusterGetCallback(const std::string& key, const evmc::GetResult& rt) {
	std::cout << key << ":" << rt.value << std::endl;
	if (0 == rt.code) {
		ConvertFromBinary(rt.value);
		std::cout << std::endl;
	} else {
		std::cout << "ret error, err code = " << rt.code << std::endl;
	}
	exit(0);
}

int main(int argc, char *argv[]) {
	::gflags::ParseCommandLineFlags(&argc, &argv, true);  
    g_loop = new evpp::EventLoop;
	std::cout << FLAGS_host << std::endl;
    auto mpc = new evmc::MemcacheClientPool(FLAGS_host.c_str(), 1, 100);
	assert(mpc);
    bool rt = mpc->Start();
	assert(rt == true);
    mpc->Get(g_loop, FLAGS_key.c_str(), std::bind(&ClusterGetCallback, std::placeholders::_1, std::placeholders::_2));
    g_loop->Run();
    return 0;
}

