#pragma once

#include "evpp/exp.h"

#include "evpp/libevent_headers.h"
#include "evpp/libevent_watcher.h"
#include "evpp/event_loop.h"
#include "evpp/event_loop_thread.h"
#include "evpp/buffer.h"
#include "evpp/tcp_conn.h"
#include "evpp/tcp_client.h"


namespace evmc {

// TODO 
// - error handling: conn, timeout
// - server addr hash / vbucket
// - embeded & standalone

typedef std::shared_ptr<evpp::Buffer> BufferPtr;
typedef std::shared_ptr<evpp::EventLoop> EventLoopPtr;
typedef std::shared_ptr<evpp::TCPClient> TcpClientPtr;
typedef std::shared_ptr<evpp::TimerEventWatcher> TimerEventPtr;

const int ERR_CODE_TIMEOUT = -1;
const int ERR_CODE_NETWORK = -2;
const int ERR_CODE_DISCONNECT = -3;

struct GetResult {
    GetResult() : code(-1) {}
    GetResult(int c, const std::string& v) : code(c), value(v) {}
    int code;
    std::string value;
};

struct MultiGetResult {
    int code;
    std::map<std::string, GetResult> get_result_map_;
};

// TODO : use reference type ?
typedef std::function<void(const std::string key, GetResult result)> GetCallback;
typedef std::function<void(const std::string key, int code)> SetCallback;
typedef std::function<void(const std::string key, int code)> RemoveCallback;
typedef std::function<void(MultiGetResult result)> MultiGetCallback;

class MemcacheClient;
typedef std::shared_ptr<MemcacheClient> MemcacheClientPtr;

}

