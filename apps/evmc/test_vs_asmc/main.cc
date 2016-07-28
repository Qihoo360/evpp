#include "cluster_case.h"
#include <event2/event.h>
#include <gflags/gflags.h>

//static const char* moxi_host = "182.118.21.171";
//static uint16_t moxi_port = 11221;
//static const char* conf = "http://182.118.21.171:8360/storage_cluster_config_failover";


//static const char* conf = "http://182.118.21.171:8360/storage_cluster_config_cleanpage";

static evpp::EventLoop* g_loop = NULL;
static int32_t g_running = false;
static void MyEventThread() {
    g_loop = new evpp::EventLoop;
	g_running = true;
    g_loop->Run();
	g_running = false;
	
}

int main(int argc, char *argv[]) {
	::gflags::ParseCommandLineFlags(&argc, &argv, true);  
    ClusterCase* cs = new ClusterCase("./kill_storage_cluster.json", 0);

    std::thread th(MyEventThread);
	th.detach();
	while (false == g_running) {
		usleep(1000);
	}
    cs->Init(g_loop);

	evpp::EventLoop * main_loop = new evpp::EventLoop;
    cs->Run(main_loop);
	cs->PrintResult();
    cs->Stop();

	delete g_loop;
	delete main_loop;
    delete cs;

    return 0;
}

