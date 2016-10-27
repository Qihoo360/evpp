#pragma once

#include "evpp/exp.h"

#include "evpp/libevent_watcher.h"
#include "evpp/event_loop_thread.h"
#include "evpp/buffer.h"
#include "evpp/tcp_conn.h"
#include "evpp/tcp_client.h"
#include "glog/logging.h"
#include "objectpool.hpp"

namespace evmc {

// TODO
// - embedded & standalone

typedef std::shared_ptr<evpp::Buffer> BufferPtr;
typedef std::shared_ptr<evpp::TimerEventWatcher> TimerEventPtr;

enum {
    ERR_CODE_TIMEOUT = -1,
    ERR_CODE_NETWORK = -2,
    ERR_CODE_DISCONNECT = -3,
    ERR_CODE_UNDEFINED = -100,
};


struct GetResult {
    GetResult() : code(ERR_CODE_UNDEFINED) {}
    GetResult(const GetResult& result) : code(result.code), value(result.value) {}
    GetResult(GetResult&& result) : code(result.code), value(std::move(result.value)) {}
    GetResult& operator=(GetResult&& result) {
		code = result.code;
		value = std::move(result.value);
		return *this;
	}

    GetResult(int c, const std::string& v) : code(c), value(v) {}
    int code;
    std::string value;
};

struct MultiGetResult {
    MultiGetResult() : code(ERR_CODE_UNDEFINED) {}
    MultiGetResult(const MultiGetResult& result) : code(result.code), get_result_map_(result.get_result_map_) {}
    MultiGetResult(MultiGetResult&& result) : code(result.code), get_result_map_(std::move(result.get_result_map_)) {}
    MultiGetResult& operator=(MultiGetResult&& result) {
		code = result.code;
		get_result_map_ = std::move(result.get_result_map_);
		return *this;
	}
    
    int code; // TODO ʹ�� enum ����
    std::map<std::string, GetResult> get_result_map_;
};

typedef std::shared_ptr<GetResult> GetResultPtr;
typedef std::map<std::string, GetResult>  MultiGetMapResult;
typedef std::shared_ptr<MultiGetMapResult> MultiGetMapResultPtr;

struct PrefixGetResult {
    PrefixGetResult() : code(ERR_CODE_UNDEFINED) {
	}
	virtual ~PrefixGetResult() {
	}
    PrefixGetResult(const PrefixGetResult& result) { 
		code = result.code;
		get_result_map_ = result.get_result_map_;   
	}
    PrefixGetResult(PrefixGetResult&& result) : code(result.code), get_result_map_(std::move(result.get_result_map_)) {
	}
	PrefixGetResult& operator=(PrefixGetResult&& result) {
		code = result.code;
		get_result_map_  = std::move(result.get_result_map_);
		return *this;
	}
    int code;
    std::map<std::string, std::string> get_result_map_;
	void clear() {
		code = ERR_CODE_UNDEFINED;
		get_result_map_.clear();
	}
};

typedef std::shared_ptr<PrefixGetResult> PrefixGetResultPtr;

struct PrefixMultiGetResult {
    PrefixMultiGetResult() : code(ERR_CODE_UNDEFINED) {
	}
    virtual ~PrefixMultiGetResult() {
	/*	if (!get_result_map_.empty()) {
			auto it = get_result_map_.begin();
			for (; it != get_result_map_.end(); ) {
				get_result_map_.erase(it++);
			}
		}*/
	}
    PrefixMultiGetResult(const PrefixMultiGetResult& result) {
		code = result.code;
		get_result_map_ = result.get_result_map_;
	}

    PrefixMultiGetResult(PrefixMultiGetResult&& result): code(result.code), 
	get_result_map_(std::move(result.get_result_map_)) {
	}

   PrefixMultiGetResult& operator=(PrefixMultiGetResult&& result) {
		code = result.code;
		get_result_map_ = std::move(result.get_result_map_);
		return *this;
	}

    int code;
    std::map<std::string, PrefixGetResultPtr> get_result_map_;
	void clear() {
		code = ERR_CODE_UNDEFINED;
		get_result_map_.clear();
	}
};
typedef std::shared_ptr<PrefixMultiGetResult> PrefixMultiGetResultPtr;


typedef std::function<void(const std::string& key, const GetResult& result)> GetCallback;
typedef std::function<void(const std::string& key, int code)> SetCallback;
typedef std::function<void(const std::string& key, int code)> RemoveCallback;
typedef std::function<void(const MultiGetResult& result)> MultiGetCallback;
typedef std::function<void(const MultiGetMapResultPtr& result, int code)> MultiGetCallback2;
typedef std::function<void(const std::string& key, const PrefixGetResultPtr result)> PrefixGetCallback;
typedef std::function<void(const PrefixMultiGetResultPtr result)> PrefixMultiGetCallback;

class MemcacheClient;
typedef std::shared_ptr<MemcacheClient> MemcacheClientPtr;

class MultiGetCollector2 {
public:
    MultiGetCollector2():caller_loop_(NULL), collect_counter_(0){}
    void Init(evpp::EventLoop* caller_loop, int count, const MultiGetCallback2& cb, const uint32_t thread_hash) {
		caller_loop_ = caller_loop;
		collect_counter_ = count;
		thread_hash_ = thread_hash;
		kvs_.reset();
		kvs_ = std::make_shared<MultiGetMapResult>();
		callback_ = cb;
	}
    void Collect(std::string & key, GetResult & result) {
		if (collect_counter_ <= 0) {
			LOG_WARN << "occur errors, repeat response";
			return;
		}
		kvs_->emplace(std::move(key), std::move(result));

        LOG_DEBUG << "MultiGetCollector2 count=" << collect_counter_;

        if (--collect_counter_ == 0) {
            if (caller_loop_) {
                //callback_(kvs_, err_code);
				//struct timeval tv;
                caller_loop_->RunInLoop(std::bind(callback_, kvs_, 0));
            } else {
                callback_(kvs_, 0);
            }
			kvs_.reset();
        }
    }
public:
	uint32_t thread_hash_;
private:
    evpp::EventLoop* caller_loop_;
    int collect_counter_;
    MultiGetMapResultPtr kvs_;
    MultiGetCallback2 callback_;
};
typedef std::shared_ptr<MultiGetCollector2> MultiGetCollector2Ptr;
typedef ObjectPool<MultiGetCollector2, MultiGetCollector2> MultiGet2CollectorPool;

}

