#pragma once

#include "evpp/exp.h"

#include "evpp/libevent_watcher.h"
#include "evpp/event_loop_thread.h"
#include "evpp/buffer.h"
#include "evpp/tcp_conn.h"
#include "evpp/tcp_client.h"

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

    int code;
    std::map<std::string, GetResult> get_result_map_;
};

struct PrefixGetResult {
    PrefixGetResult() : code(ERR_CODE_UNDEFINED) {}
    PrefixGetResult(const PrefixGetResult& result) { 
		code = result.code;
		get_result_map_ = result.get_result_map_;   
	}
    PrefixGetResult(PrefixGetResult&& result) : code(result.code), get_result_map_(std::move(result.get_result_map_)) {}
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

struct PrefixMultiGetResult {
    PrefixMultiGetResult() : code(ERR_CODE_UNDEFINED) {}
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
	std::map<std::string, PrefixGetResult> get_result_map_;
	void clear() {
		code = ERR_CODE_UNDEFINED;
		get_result_map_.clear();
	}
};

typedef std::function<void(const std::string& key, const GetResult& result)> GetCallback;
typedef std::function<void(const std::string& key, int code)> SetCallback;
typedef std::function<void(const std::string& key, int code)> RemoveCallback;
typedef std::function<void(const MultiGetResult& result)> MultiGetCallback;
typedef std::function<void(const std::string& key, const PrefixGetResult& result)> PrefixGetCallback;
typedef std::function<void(PrefixMultiGetResult& result)> PrefixMultiGetCallback;

class MemcacheClient;
typedef std::shared_ptr<MemcacheClient> MemcacheClientPtr;
}

