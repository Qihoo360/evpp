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

enum EvmcCode {
    EVMC_SUCCESS = 0x00,
    EVMC_KEY_ENOENT = 0x01,
    EVMC_KEY_EEXISTS = 0x02,
    EVMC_E2BIG = 0x03,
    EVMC_EINVAL = 0x04,
    EVMC_NOT_STORED = 0x05,
    EVMC_DELTA_BADVAL = 0x06,
    EVMC_NOT_MY_VBUCKET = 0x07,
    EVMC_AUTH_ERROR = 0x20,
    EVMC_AUTH_CONTINUE = 0x21,
    EVMC_ERANGE = 0x22,
    EVMC_UNKNOWN_COMMAND = 0x81,
    EVMC_ENOMEM = 0x82,
    EVMC_NOT_SUPPORTED = 0x83,
    EVMC_EINTERNAL = 0x84,
    EVMC_EBUSY = 0x85,
    EVMC_ETMPFAIL = 0x86,

    EVMC_TIMEOUT = -1,
    EVMC_NETWORK = -2,
    EVMC_DISCONNECT = -3,
    EVMC_ALLDOWN = -4,
    EVMC_EXCEPTION = -5,
};


struct GetResult {
    GetResult() : code(EVMC_KEY_ENOENT) {}
    GetResult(const GetResult& result) : code(result.code), value(result.value) {}
    GetResult(GetResult&& result) : code(result.code), value(std::move(result.value)) {}
    GetResult& operator=(GetResult&& result) {
        code = result.code;
        value = std::move(result.value);
        return *this;
    }

    GetResult(const int c, const std::string& v) : code(c), value(v) {}
    void set(const int c, std::string&& v) {
        code = c;
        value = std::move(v);
    }
    void error(const int c) {
        code = c;
    }

    int code;
    std::string value;
};

typedef std::shared_ptr<GetResult> GetResultPtr;
typedef std::map<std::string, GetResult>  MultiGetResult;

typedef std::function<void(const MultiGetResult& result)> MultiGetCallback;
typedef std::function<void(const std::string& key, int code)> SetCallback;
typedef std::function<void(const std::string& key, int code)> RemoveCallback;
typedef std::function<void(const std::string& key, const GetResult& result)> GetCallback;

struct PrefixGetResult {
    PrefixGetResult() : code(EVMC_KEY_ENOENT) {
    }

    virtual ~PrefixGetResult() {
    }

    PrefixGetResult(const PrefixGetResult& result) {
        code = result.code;
        result_map = result.result_map;
    }

    PrefixGetResult(PrefixGetResult&& result) : code(result.code), result_map(std::move(result.result_map)) {
    }

    PrefixGetResult& operator=(PrefixGetResult&& result) {
        code = result.code;
        result_map = std::move(result.result_map);
        return *this;
    }
    void set(const int c, std::map<std::string, std::string>&& r_map) {
        code = c;
        result_map = std::move(r_map);
    }
    inline void error(const int c) {
        code = c;
    }
    int code;
    std::map<std::string, std::string> result_map;
};

typedef std::shared_ptr<PrefixGetResult> PrefixGetResultPtr;
typedef std::function<void(const std::string& key, const PrefixGetResultPtr result)> PrefixGetCallback;

typedef std::map<std::string, PrefixGetResultPtr> PrefixMultiGetResult;
typedef std::function<void(const PrefixMultiGetResult& result)> PrefixMultiGetCallback;

class MemcacheClient;
typedef std::shared_ptr<MemcacheClient> MemcacheClientPtr;

}

