#include <iostream>
#include <gflags/gflags.h>
#include <glog/logging.h>
#include <sys/time.h>
#include <sys/types.h>
#include <algorithm>
#include <event2/event.h>
#include "cluster_case.h"
#include "evmc/exp.h"
#include "evmc/memcache_client_pool.h"
#include "evmc/vbucket_config.h"

DEFINE_int32(thread_nums, 10, "thread pool size");
DEFINE_int32(timeout_ms, 500, "read or write timeout");
DEFINE_int32(key_size, 12, "memecached test key size");
DEFINE_int32(packet_nums, 1000, "send total request nums to memcached");
DEFINE_int32(qps, 1000, "send request packets per second"); 
DEFINE_int32(test_type, 0, "0:set 1:get 2:MultiGet 3:remove"); 
DEFINE_int32(multiget_nums, 5, "multiget key nums per transaction"); 
DEFINE_int32(prefixmultiget_nums, 5, "prefixmultiget key nums per transaction"); 

ClusterCase::ClusterCase(const std::string& conf, std::size_t total)
    : conf_(conf)
    , event_loop_(NULL)
    , event_loop_main_(NULL)
    , mpc_(NULL)
    , total_(total)
    , cursor_(0)
    , error_nums_(0) 
    , index_(0) {
}

bool ClusterCase::Init(evpp::EventLoop * event) {
    bool rt = false;
    event_loop_ = event;
    mpc_ = new evmc::MemcacheClientPool("./kill_storage_cluster.json", FLAGS_thread_nums, FLAGS_timeout_ms);
    rt = mpc_->Start();
	assert(rt == true);
    return rt;
}

bool ClusterCase::Stop() {
	mpc_->Stop(true);
    delete mpc_;
    return true;
}

void ClusterCase::RunTask(const int packets_num_per_cycle) {
	std::string key;
	std::vector<std::string> keys;
	int32_t k;
	if (index_ >= FLAGS_packet_nums) {
		return;
	}
	
	std::ostringstream os;
	for (int j = 0; j < packets_num_per_cycle; ++j) {
		switch (FLAGS_test_type) {
			case 0:
				MakeData(key, index_++, FLAGS_key_size);
				ClusterSet(key, key);
				break;
			case 1:
				MakeData(key, index_++, FLAGS_key_size);
				ClusterGet(key);
				break;
			case 2:
				k = index_++;
				keys.clear();
				for (int num = 0; num < FLAGS_multiget_nums; ++num) {
					MakeData(key, k++, FLAGS_key_size);
					keys.push_back(key);
				}
				ClusterMultiGet(keys);
				break;
			case 3:
				MakeData(key, index_++, FLAGS_key_size);
				ClusterRemove(key);
				break;
			case 4:
				os.str("");
				os << index_++;
				ClusterPrefixGet(os.str());
				break;
			case 5:
				k = index_++;
				keys.clear();
				for (int num = 0; num < FLAGS_prefixmultiget_nums; ++num) {
					os.str("");
					os << k++;
					keys.push_back(os.str());
					//LOG_INFO << "begin to call key=" << os.str();
				}
				ClusterPrefixMultiGet(keys);
				break;
			case -1:
				k = index_;
				keys.clear();
				MakeData(key, index_++, FLAGS_key_size);
				ClusterSet(key, key);
				ClusterGet(key);
				for (int num = 0; num < FLAGS_multiget_nums; ++num) {
					MakeData(key, k++, FLAGS_key_size);
					keys.push_back(key);
				}
				ClusterMultiGet(keys);
				ClusterPrefixGet(key);
				ClusterPrefixMultiGet(keys);
				ClusterRemove(key);
				break;
			}

			if (index_ >= FLAGS_packet_nums) {
				break;
			}
	}
}

void ClusterCase::PrintResult() {
	const int32_t size = cost_list_.size();
	std::cout << "key size:" << FLAGS_key_size << ",request nums:" << FLAGS_packet_nums
		<< ",qps:" << FLAGS_qps << ",type:" << FLAGS_test_type << ",multiget key size:" << FLAGS_multiget_nums << std::endl;
	std::cout << "success samples:" << size - error_nums_ << ",error samples:" << error_nums_ << "query success samples:" << total_ <<  std::endl;
	if (0 >= size) {
		return;
	}
	std::sort(cost_list_.begin(), cost_list_.end());
	std::vector<int64_t>::iterator it = cost_list_.begin();
	int64_t sum = 0;
	for (; it != cost_list_.end(); ++it) {
		sum += *it;
//		std::cout << "sum time " << *it << std::endl;
	}
	std::cout << "min cost:" << cost_list_[0] << " max cost:" << cost_list_[size - 1]  
		<< " avg cost:" << double(sum) / double(cost_list_.size()) << std::endl;
}



void ClusterCase::Run(evpp::EventLoop * event_loop) {
	int packets_num_per_cycle = 1;
	int mssleep_idl = 10;
	if (FLAGS_qps > 100) {
		mssleep_idl = 10;
		packets_num_per_cycle = FLAGS_qps % 10? (FLAGS_qps + 10) / 10 : FLAGS_qps / 10; 
	} else {
		mssleep_idl = 1000 / FLAGS_qps;
		packets_num_per_cycle = 1;
	}
	event_loop_main_ = event_loop;
	event_loop_main_->RunEvery(evpp::Duration(1.0), std::bind(&ClusterCase::RunTask, this, FLAGS_qps));
	event_loop_main_->Run();
   //event_loop_loopexit(event_base_, NULL);
}

bool ClusterCase::MakeData(std::string& key, const int32_t index,  const std::size_t size) {
	key.clear();
    key.assign(NumberToString<std::size_t>(index));
    if (key.size() < size) {
       key.resize(size, 'T');
    }
    return true;
}


/*
 * cluster memcached test case
 */
void ClusterCase::ClusterSet(const std::string& key, const std::string& val) {
    if (NULL == mpc_) {
        return;
    }
    Timer t;
    t.Begin();
    mpc_->Set(event_loop_, key.c_str(), val.c_str(), std::bind(&ClusterCase::ClusterSetCallback,
                this, t, std::placeholders::_1, std::placeholders::_2));
}

void ClusterCase::ClusterSetCallback(Timer t, const std::string& key, int code) {
	t.End();
	if (0 != code) {
		error_nums_++;
		LOG(WARNING) << "Set error, key:" << key << " error code=" << code;
		IncrCompletedNums(); 
		return;
	}
    cost_list_.push_back(t.Elapsedus());
	IncrCompletedNums(); 
	return;
}

void ClusterCase::ClusterGet(const std::string& key) {
    if (NULL == mpc_) {
        return;
    }
    Timer t;
    t.Begin();
    mpc_->Get(event_loop_, key.c_str(), std::bind(&ClusterCase::ClusterGetCallback,
                this, t, std::placeholders::_1, std::placeholders::_2));
}

void ClusterCase::ClusterGetCallback(Timer t, const std::string& key, const evmc::GetResult& rt) {
	t.End();
    cost_list_.push_back(t.Elapsedus());
	if (0 != rt.code || 0 != key.compare(rt.value)) {
		error_nums_++;
		LOG(WARNING) << "Get ret error, retcode=" << rt.code << ",key=" << key << ",value=" << rt.value;
		IncrCompletedNums(); 
		return;
	}
	total_ += 1;
	IncrCompletedNums(); 
}

void ClusterCase::ClusterMultiGet(const std::vector<std::string>& keys) {
    if (NULL == mpc_) {
        return;
    }
	Timer t;
	t.Begin();
    mpc_->MultiGet(event_loop_, keys, std::bind(&ClusterMultiGetCallback, this, t, std::placeholders::_1));
}

void ClusterCase::ClusterMultiGetCallback(Timer t, const  evmc::MultiGetResult & m) {
	t.End();
    const std::map<std::string, evmc::GetResult>& rts =  m.get_result_map_;
    std::map<std::string, evmc::GetResult>::const_iterator it = rts.begin();
    std::map<std::string, evmc::GetResult>::const_iterator end = rts.end();
    for ( ; it != end; ++it) {
		const evmc::GetResult& rt = it->second;
		if (0 != rt.code || 0 != (it->first).compare(rt.value)) {
			LOG(WARNING) << "MultiGet ret error, rtcode=" << rt.code << " key=" << it->first << ",value=" << rt.value;
			error_nums_++;
			continue;
		} else {
			total_ += 1;
			LOG(INFO) << "MultiGet ret success, rtcode=" << rt.code << " key=" << it->first << ",value=" << rt.value;
		}
	}
    cost_list_.push_back(t.Elapsedus());
	IncrCompletedNums();
}

void ClusterCase::ClusterRemove(const std::string& key) {
    if (NULL == mpc_) {
        return;
    }
	Timer t;
	t.Begin();
    mpc_->Remove(event_loop_, key.c_str(), std::bind(&ClusterCase::ClusterRemoveCallback, this, t, std::placeholders::_1,std::placeholders::_2));
}

void ClusterCase::ClusterRemoveCallback(Timer t, const std::string& key, int code) {
	t.End();
	if (0 != code) {
		error_nums_++;
		IncrCompletedNums(); 
		return;
	}
    cost_list_.push_back(t.Elapsedus());
	IncrCompletedNums();
}

void ClusterCase::ClusterPrefixGet(const std::string& key) {
    if (NULL == mpc_) {
        return;
    }
    Timer t;
    t.Begin();
    mpc_->PrefixGet(event_loop_, key.c_str(), std::bind(&ClusterCase::ClusterPrefixGetCallback,
                this, t, std::placeholders::_1, std::placeholders::_2));
}

void ClusterCase::ClusterPrefixGetCallback(Timer t, const std::string& key, const evmc::PrefixGetResultPtr rt) {
	t.End();
    cost_list_.push_back(t.Elapsedus());
	//LOG_INFO << "OnPrefixGet success key=" << key;
	if (0 != rt->code) {
		LOG(WARNING) << "PrefixGet error key=" << key << " ret, retcode=" << rt->code;
		error_nums_++;
	} else {
		auto & result_map = rt->get_result_map_;
		for (auto it = result_map.begin(); it != result_map.end(); ++it) {
			if (0 != it->first.compare(it->second)) {
				LOG(WARNING) << "PrefixGet error key=" << key << " , key=" << it->first << " value=" << it->second;
			} else {
				total_ += 1;
				//LOG_INFO << "PrefixGet success key=" << key << " , key=" << it->first << " value=" << it->second;
			}
		}
	}
	IncrCompletedNums(); 
}

void ClusterCase::ClusterPrefixMultiGet(const std::vector<std::string>& keys) {
    if (NULL == mpc_) {
        return;
    }
	Timer t;
	t.Begin();
    mpc_->PrefixMultiGet(event_loop_, keys, std::bind(&ClusterPrefixMultiGetCallback, this, t, std::placeholders::_1));
}

void ClusterCase::ClusterPrefixMultiGetCallback(Timer t, const evmc::PrefixMultiGetResultPtr m) {
	t.End();
    cost_list_.push_back(t.Elapsedus());
	//LOG_INFO << "time key=" << t.Elapsedus();
	if (m->code != 0) {
		LOG(WARNING) << "PrefixMultiGet error, retcode =" << m->code;
		error_nums_++;
	} else {
		const auto & rts =  m->get_result_map_;
		for (auto it = rts.begin(); it != rts.end(); ++it) {
			auto & result_map = it->second->get_result_map_;
			for (auto iter = result_map.begin(); iter != result_map.end(); ++iter) {
				if (0 != iter->first.compare(iter->second)) {
					LOG(WARNING) << "PrefixMultiGet error key=" << it->first << " , key=" << iter->first << " value=" << iter->second;
				} else {
					total_ += 1;
			//		LOG_INFO << "PrefixMultiGet success key=" << it->first << " time=" << t.Elapsedus() << " , key=" << iter->first << " value=" << iter->second;
				}
			}
		}
	}
	IncrCompletedNums();
}

void ClusterCase::IncrCompletedNums() {
	int ret = completed_task_num_.fetch_add(1, std::memory_order_relaxed);
	if ((ret + 1)  >= FLAGS_packet_nums) {
		sleep(1);
		event_loop_->Stop();
		event_loop_main_->Stop();
	}
}

