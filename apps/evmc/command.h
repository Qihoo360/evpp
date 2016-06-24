#pragma once

#include "mctypes.h"

namespace evmc {

class Command  : public std::enable_shared_from_this<Command> {
public:
    Command(EventLoopPtr evloop, uint16_t vbucket, uint32_t th_hash)
            : caller_loop_(evloop), id_(0)
            , vbucket_id_(vbucket), thread_hash_(th_hash) {
    }

    uint32_t id() const { return id_; }
    uint32_t thread_hash() const { return thread_hash_; }
    uint16_t vbucket_id() const { return vbucket_id_; }
    EventLoopPtr caller_loop() const { return caller_loop_; }

    void Launch(MemcacheClientPtr memc_client);

    virtual void OnError(int code) = 0;
    virtual void OnSetCommandDone(int resp_code) {}
    virtual void OnRemoveCommandDone(int resp_code) {}
    virtual void OnGetCommandDone(int resp_code, const std::string& value) {}
    virtual void OnMultiGetCommandOneResponse(int resp_code, const std::string& key, const std::string& value) {}
    virtual void OnMultiGetCommandDone(int resp_code, const std::string& key, const std::string& value) {}

private:
    virtual BufferPtr RequestBuffer() const = 0;
    EventLoopPtr caller_loop_;
    uint32_t id_;               // 并非全局id，只是各个memc_client内部的序号; mget的多个命令公用一个id
    uint16_t vbucket_id_;
    uint32_t thread_hash_;
};
typedef std::shared_ptr<Command> CommandPtr;

class SetCommand  : public Command {
public:
    SetCommand(EventLoopPtr evloop, uint16_t vbucket, const char* key, const char * value, size_t val_len,
               uint32_t flags, uint32_t expire, SetCallback callback)
           : Command(evloop, vbucket, rand()), key_(key), value_(value, val_len),
             flags_(flags), expire_(expire),
             set_callback_(callback) {
    }

    virtual void OnError(int err_code) {
        if (caller_loop()) {
            caller_loop()->RunInLoop(std::bind(set_callback_,
                        key_, err_code));
        } else {
            set_callback_(key_, err_code);
        }
    }
    virtual void OnSetCommandDone(int resp_code) {
        if (caller_loop()) {
            caller_loop()->RunInLoop(std::bind(set_callback_,
                        key_, resp_code));
        } else {
            set_callback_(key_, resp_code);
        }
    }
private:
    std::string key_;
    std::string value_;
    uint32_t flags_;
    uint32_t expire_;
    SetCallback set_callback_;
private:
    virtual BufferPtr RequestBuffer() const;
};

class GetCommand  : public Command {
public:
    GetCommand(EventLoopPtr evloop, uint16_t vbucket, const char* key, GetCallback callback) 
            : Command(evloop, vbucket, rand())
            , key_(key)
            , get_callback_(callback) {
    }

    virtual void OnError(int err_code) {
        if (caller_loop()) {
            caller_loop()->RunInLoop(std::bind(get_callback_, key_,
                        GetResult(err_code, std::string())));
        } else {
            get_callback_(key_, GetResult(err_code, std::string()));
        }
    }
    virtual void OnGetCommandDone(int resp_code, const std::string& value) {
        if (caller_loop()) {
            caller_loop()->RunInLoop(std::bind(get_callback_, key_,
                        GetResult(resp_code, value)));
        } else {
            get_callback_(key_, GetResult(resp_code, value));
        }
    }
private:
    std::string key_;
    GetCallback get_callback_;
private:
    virtual BufferPtr RequestBuffer() const;
};


class MultiGetCommand  : public Command {
public:
    MultiGetCommand(EventLoopPtr evloop, uint16_t vbucket, uint32_t th_hash, const std::vector<std::string>& keys, MultiGetCallback callback)
            : Command(evloop, vbucket, th_hash), keys_(keys), mget_callback_(callback) {
    }

    virtual void OnError(int err_code) {
        mget_result_.code = err_code;
        if (caller_loop()) {
            caller_loop()->RunInLoop(std::bind(mget_callback_, mget_result_));
        } else {
            mget_callback_(mget_result_);
        }
    }
    virtual void OnMultiGetCommandDone(int resp_code, const std::string& key, const std::string& value);
    virtual void OnMultiGetCommandOneResponse(int resp_code, const std::string& key, const std::string& value);
private:
    std::vector<std::string> keys_;
    MultiGetCallback mget_callback_;
    MultiGetResult mget_result_;
private:
    virtual BufferPtr RequestBuffer() const;
};

class RemoveCommand  : public Command {
public:
    RemoveCommand(EventLoopPtr evloop, uint16_t vbucket, const char* key, RemoveCallback callback)
           : Command(evloop, vbucket, rand()), key_(key), remove_callback_(callback) {
    }
    virtual void OnError(int err_code) {
        if (caller_loop()) {
            caller_loop()->RunInLoop(std::bind(remove_callback_, key_, err_code));
        } else {
            remove_callback_(key_, err_code);
        }
    }
    virtual void OnRemoveCommandDone(int resp_code) {
        if (caller_loop()) {
            caller_loop()->RunInLoop(std::bind(remove_callback_, key_, resp_code));
        } else {
            remove_callback_(key_, resp_code);
        }
    }
private:
    std::string key_;
    RemoveCallback remove_callback_;
private:
    virtual BufferPtr RequestBuffer() const;
};

}

