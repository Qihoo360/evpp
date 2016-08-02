#ifndef __CLUSTER_CASE_H__
#define __CLUSTER_CASE_H__


#include <map>
#include <string>
#include <sstream>

#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include "evmc/exp.h"
#include "evmc/memcache_client_pool.h"
#include "evmc/vbucket_config.h"

class Timer {
public:
	Timer() {
		begin_ = {0,0};
		end_ = {0,0};
	}
	Timer(const Timer& t) {
		begin_ = t.begin_;
		end_ = t.end_;
	}
	int64_t GetTimestampMs() {
		struct timeval tv;
		gettimeofday(&tv, NULL);
		return tv.tv_sec * 1000 + tv.tv_usec / 1000;
	}
	inline void Begin() {
		gettimeofday(&begin_, NULL);
	}

	inline void End() {
		gettimeofday(&end_, NULL);
	}
	static void MsSleep(const int64_t t) {
		struct timeval tv;
		tv.tv_sec = t / 1000;
		tv.tv_usec = (t % 1000) * 1000;
		int err = 0;
		do {
			err = select(0, NULL, NULL, NULL, &tv);
		}while (err < 0 && errno == EINTR);
	}
	inline int64_t Elapsedus() {
		return (end_.tv_sec - begin_.tv_sec) * 1e6 + (end_.tv_usec - begin_.tv_usec);  
	}
          
private:
	struct timeval begin_;
	struct timeval end_;
};

class ClusterCase {
public:
    ClusterCase(const std::string& conf, std::size_t total);
    typedef evpp::EventLoop* EventLoopPtr;
    bool Init(evpp::EventLoop* event);
    bool Stop();

    void Run(evpp::EventLoop *);
    bool MakeData(std::string& key, const int32_t index, const std::size_t size);
	void PrintResult();
	void RunTask(int packages_num_per_cycle); 

private:
    template <typename T>
    std::string NumberToString(T number) {
        std::ostringstream oss;
        oss << number;

        return oss.str();
    }

    /*
     * cluster memcached test case
     */

	void ClusterSet(const std::string& key, const std::string& val);
	void ClusterSetCallback(Timer t, const std::string& key, int code);
	void ClusterGet(const std::string& key);
	void ClusterGetCallback(Timer t, const std::string& key, const evmc::GetResult& rt);
	void ClusterMultiGet(const std::vector<std::string>& keys);
	void ClusterMultiGetCallback(Timer t, const evmc::MultiGetResult& m);
	void ClusterRemove(const std::string& key);
	void ClusterRemoveCallback(Timer t, const std::string&key, int code);

	void ClusterPrefixGet(const std::string& key);
	void ClusterPrefixGetCallback(Timer t, const std::string& key, const evmc::PrefixGetResultPtr rt);

	void ClusterPrefixMultiGet(const std::vector<std::string>& keys);
	void ClusterPrefixMultiGetCallback(Timer t, const evmc::PrefixMultiGetResultPtr m);

	void IncrCompletedNums(); 

private:
    std::string conf_;
    EventLoopPtr event_loop_;
    EventLoopPtr event_loop_main_;
	evmc::MemcacheClientPool* mpc_;

    std::size_t total_;
    std::size_t cursor_;
    std::vector<int64_t> cost_list_;
	int32_t error_nums_;
    int32_t index_;
	std::atomic<int> completed_task_num_;
};
#endif /* __CLUSTER_CASE_H__ */

