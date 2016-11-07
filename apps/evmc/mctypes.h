#pragma once

#include "evpp/exp.h"

#include "evpp/libevent_watcher.h"
#include "evpp/event_loop_thread.h"
#include "evpp/buffer.h"
#include "evpp/tcp_conn.h"
#include "evpp/tcp_client.h"
#include "glog/logging.h"
//#include "objectpool.hpp"

namespace evmc {

// TODO
// - embedded & standalone

typedef std::shared_ptr<evpp::Buffer> BufferPtr;
typedef std::shared_ptr<evpp::TimerEventWatcher> TimerEventPtr;

enum {
	SUC_CODE = 0,
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

template <class T>
struct MultiGetResultT {
    MultiGetResultT() : code(ERR_CODE_UNDEFINED) {}
    MultiGetResultT(const MultiGetResultT& result) : code(result.code), get_result_map_(result.get_result_map_) { }
    MultiGetResultT(MultiGetResultT&& result) : code(result.code), get_result_map_(std::move(result.get_result_map_)) {}
    MultiGetResultT& operator=(MultiGetResultT&& result) {
		code = result.code;
		get_result_map_ = std::move(result.get_result_map_);
		return *this;
	}
    
    int code; // TODO 使用 enum 变量
    std::map<std::string, T> get_result_map_;
};

typedef std::shared_ptr<GetResult> GetResultPtr;
typedef std::map<std::string, GetResult>  MultiGetMapResult;
typedef std::shared_ptr<MultiGetMapResult> MultiGetMapResultPtr;
typedef MultiGetResultT<GetResult> MultiGetResult;

struct PrefixGetResult {
    PrefixGetResult() : code(ERR_CODE_UNDEFINED) {
	}
	virtual ~PrefixGetResult() {
	}
    PrefixGetResult(const PrefixGetResult& result) { 
		code = result.code;
		result_map_ = result.result_map_;   
	}
    PrefixGetResult(PrefixGetResult&& result) : code(result.code), result_map_(std::move(result.result_map_)) {
	}
	PrefixGetResult& operator=(PrefixGetResult&& result) {
		code = result.code;
		result_map_ = std::move(result.result_map_);
		return *this;
	}
    int code;
    std::map<std::string, std::string> result_map_;
	void clear() {
		code = ERR_CODE_UNDEFINED;
		result_map_.clear();
	}
};

typedef std::shared_ptr<PrefixGetResult> PrefixGetResultPtr;

typedef MultiGetResultT<PrefixGetResultPtr> PrefixMultiGetResult;


typedef std::function<void(const std::string& key, const GetResult& result)> GetCallback;
typedef std::function<void(const std::string& key, int code)> SetCallback;
typedef std::function<void(const std::string& key, int code)> RemoveCallback;
typedef std::function<void(const MultiGetResultT<GetResult>& result)> MultiGetCallback;
typedef std::function<void(const MultiGetMapResultPtr& result, int code)> MultiGetCallback2;
typedef std::function<void(const std::string& key, const PrefixGetResultPtr result)> PrefixGetCallback;
typedef std::function<void(const PrefixMultiGetResult& result)> PrefixMultiGetCallback;

class MemcacheClient;
typedef std::shared_ptr<MemcacheClient> MemcacheClientPtr;

}

