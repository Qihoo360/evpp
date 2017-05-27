#pragma once

#include "evpp/event_watcher.h"
#include "evpp/event_loop_thread.h"
#include "evpp/buffer.h"
#include "evpp/tcp_conn.h"
#include "evpp/tcp_client.h"

#include "evmc/config.h"

namespace evmc {

// TODO
// - embedded & standalone

typedef std::shared_ptr<evpp::TimerEventWatcher> TimerEventPtr;

enum {
    SUC_RET = 0,
    NOT_FIND_RET = 1,
    ERR_CODE_TIMEOUT = -1,
    ERR_CODE_NETWORK = -2,
    ERR_CODE_DISCONNECT = -3,
    ERR_CODE_EMPTYKEY = -4,
};


struct GetResult {
    GetResult() : code(NOT_FIND_RET) {}
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

typedef std::shared_ptr<GetResult> GetResultPtr;
typedef std::map<std::string, GetResult>  MultiGetResult;

struct PrefixGetResult {
    PrefixGetResult() : code(NOT_FIND_RET) {
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
        code = NOT_FIND_RET;
        result_map_.clear();
    }
};

typedef std::shared_ptr<PrefixGetResult> PrefixGetResultPtr;

typedef std::map<std::string, PrefixGetResultPtr> PrefixMultiGetResult;


typedef std::function<void(const std::string& key, const GetResult& result)> GetCallback;
typedef std::function<void(const std::string& key, int code)> SetCallback;
typedef std::function<void(const std::string& key, int code)> RemoveCallback;
typedef std::function<void(const MultiGetResult& result)> MultiGetCallback;
typedef std::function<void(const std::string& key, const PrefixGetResultPtr result)> PrefixGetCallback;
typedef std::function<void(const PrefixMultiGetResult& result)> PrefixMultiGetCallback;
class MemcacheClient;
typedef std::shared_ptr<MemcacheClient> MemcacheClientPtr;

}

