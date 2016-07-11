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
    GetResult(int c, const std::string& v) : code(c), value(v) {}
    int code;
    std::string value;
};

struct MultiGetResult {
    MultiGetResult() : code(ERR_CODE_UNDEFINED) {}
    int code;
    std::map<std::string, GetResult> get_result_map_;
};

typedef std::function<void(const std::string& key, const GetResult& result)> GetCallback;
typedef std::function<void(const std::string& key, int code)> SetCallback;
typedef std::function<void(const std::string& key, int code)> RemoveCallback;
typedef std::function<void(const MultiGetResult& result)> MultiGetCallback;

class MemcacheClient;
typedef std::shared_ptr<MemcacheClient> MemcacheClientPtr;


}

