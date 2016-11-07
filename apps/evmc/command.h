#pragma once

#include "mctypes.h"
#include <glog/logging.h>

namespace evmc {
	/*enum CommandType {
		onekey = 0,
		redundantkey = 1,
	};*/

class Command  : public std::enable_shared_from_this<Command> {
public:
    Command(evpp::EventLoop* evloop, uint16_t vbucket)
        : caller_loop_(evloop), id_(0)
        , vbucket_id_(vbucket) /*, type_(CommandType::onekey)*/ {
    }

    virtual ~Command() {}

    inline uint32_t id() const {
        return id_;
    }
    inline void set_id(uint32_t v) {
        id_ = v;
    }

    inline uint16_t vbucket_id() const {
        return vbucket_id_;
    }

     uint16_t server_id()  const; 
    inline void set_server_id(uint16_t sid) {
        server_id_history_.emplace_back(sid);
    }

    inline evpp::EventLoop* caller_loop() const {
        return caller_loop_;
    }

    void Launch(MemcacheClientPtr memc_client);

    bool ShouldRetry() const;

    virtual void OnError(int code) = 0;
    virtual void OnSetCommandDone(int resp_code) {}
    virtual void OnRemoveCommandDone(int resp_code) {}
    virtual void OnGetCommandDone(int resp_code, const std::string& value) {}
    virtual void OnMultiGetCommandOneResponse(int resp_code, std::string& key, std::string& value) {}
    virtual void OnMultiGetCommandDone(int resp_code, std::string& key, std::string& value) {}
    virtual void OnPrefixGetCommandDone() {}
	virtual PrefixGetResultPtr& GetResultContainerByKey(const std::string& key) { return *((PrefixGetResultPtr *)0); }

private:
    virtual BufferPtr RequestBuffer() = 0;
    evpp::EventLoop* caller_loop_;
    uint32_t id_;               // 并非全局id，只是各个memc_client内部的序号; mget的多个命令公用一个id
    uint16_t vbucket_id_;
    std::vector<uint16_t> server_id_history_;        // 执行时从多个备选server中所选定的server
};

typedef std::shared_ptr<Command> CommandPtr;

class SetCommand  : public Command {
public:
    SetCommand(evpp::EventLoop* evloop, uint16_t vbucket, const std::string& key, const std::string& value,
               uint32_t flags, uint32_t expire, SetCallback callback)
        : Command(evloop, vbucket), key_(key), value_(value),
          flags_(flags), expire_(expire),
          set_callback_(callback) {
    }

    virtual void OnError(int err_code) {
        LOG_INFO << "SetCommand OnError id=" << id();

        if (caller_loop()) {
            caller_loop()->RunInLoop(std::bind(set_callback_,
                                               key_, err_code));
        } else {
            set_callback_(key_, err_code);
        }
    }
    virtual void OnSetCommandDone(int resp_code) {
        if (caller_loop()) {
            caller_loop()->RunInLoop(std::bind(set_callback_, key_, resp_code));
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
    static std::atomic_int next_thread_;
private:
    virtual BufferPtr RequestBuffer() ;
};

class GetCommand : public Command {
public:
    GetCommand(evpp::EventLoop* evloop, uint16_t vbucket, const std::string& key, GetCallback callback)
        : Command(evloop, vbucket)
        , key_(key)
        , get_callback_(callback) {
    }

    virtual void OnError(int err_code) {
        LOG(WARNING) << "GetCommand OnError id=" << id();

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
    static std::atomic_int next_thread_;
private:
    virtual BufferPtr RequestBuffer() ;
};

class PrefixGetCommand  : public Command {
public:
    PrefixGetCommand(evpp::EventLoop* evloop, uint16_t vbucket, const std::string& key, PrefixGetCallback callback)
        : Command(evloop, vbucket), key_(key), mget_callback_(callback), mget_result_(new PrefixGetResult()) {
    }

    virtual void OnError(int err_code) {
        LOG(WARNING) << "PrefixGetCommand OnError id=" << id();
        mget_result_->code = err_code;

        if (caller_loop()) {
            caller_loop()->RunInLoop(std::bind(mget_callback_, key_, mget_result_));
        } else {
            mget_callback_(key_, mget_result_);
        }
    }
    virtual void OnPrefixGetCommandDone();
	virtual PrefixGetResultPtr& GetResultContainerByKey(const std::string& key) {return mget_result_;};
private:
    std::string key_;
    PrefixGetCallback mget_callback_;
    PrefixGetResultPtr mget_result_;
    static std::atomic_int next_thread_;
private:
    virtual BufferPtr RequestBuffer();
};


template<class Result, class Callback>
class MultiKeyHandler {
	public:
		MultiKeyHandler(const Callback& callback, 
				uint32_t thread_ticket = 0, uint32_t finished_nums = 0) 
			:thread_ticket_(thread_ticket), finished_vbucket_nums_(finished_nums), vbucket_size_(0),
			mget_callback_(callback) {
			}

		inline void set_vbucket_keys(std::map<uint16_t, std::vector<std::string> >& vbucket_keys) {
			vbucket_keys_.swap(vbucket_keys);
			vbucket_size_ = vbucket_keys_.size();
		}

		inline uint32_t finished_vbucket_nums() {
			return finished_vbucket_nums_;
		}

		inline const uint32_t  FinishedOne()  {
			return finished_vbucket_nums_++;
		}

		inline const std::size_t vbucket_size() const {
			return vbucket_size_;
		}

		std::vector<std::string>& FindKeysByid(const int16_t id) {
			auto it = vbucket_keys_.find(id);
			assert(it != vbucket_keys_.end());
			return it->second;
		}

		inline Callback& get_callback() {
			return mget_callback_;
		}

		inline Result& get_result() {
			return mget_result_;
		}

		inline std::map<uint16_t, std::vector<std::string> >& get_vbucket_keys() {
			return vbucket_keys_; 
		}

	private:
		std::map<uint16_t, std::vector<std::string> > vbucket_keys_;
		uint32_t thread_ticket_;
		std::atomic_uint finished_vbucket_nums_;
		std::size_t vbucket_size_;
		Callback mget_callback_;
		Result mget_result_;
};

template<class Result, class Callback>
class MultiKeyMode {
	private:
		typedef std::shared_ptr<MultiKeyHandler<Result, Callback>> MultiKeyHandlerPtr;
	public:
		MultiKeyMode(const MultiKeyHandlerPtr& handler)
			:multikey_handler_(handler), retry_stage_(false) {
		}
		inline MultiKeyHandlerPtr& get_handler() {
			return multikey_handler_;
		}
		inline void do_retry() {
			retry_stage_ = true;
		}
		inline bool is_retry_state() {
			return retry_stage_;
		}
	private:
		MultiKeyHandlerPtr multikey_handler_;
		bool retry_stage_;
};

typedef std::shared_ptr<MultiKeyHandler<MultiGetResult, MultiGetCallback>> MultiKeyGetHandlerPtr;

class MultiGetCommand  : public Command, public MultiKeyMode<MultiGetResult, MultiGetCallback> {
public:
    MultiGetCommand(evpp::EventLoop* evloop, const uint16_t vbucket, const MultiKeyGetHandlerPtr & handler)
        :Command(evloop, vbucket), MultiKeyMode(handler) {
    }

    virtual void OnError(int err_code) {
<<<<<<< HEAD
		LOG(WARNING) << "MultiGetCommand OnError id=" << id();
		auto & keys = get_handler()->FindKeysByid(vbucket_id());
		auto & result_map = get_handler()->get_result().get_result_map_;
		auto k = result_map.begin();
		for (auto it : keys) {
			k = result_map.find(it);
			assert(k != result_map.end());
			k->second.code = err_code;
=======
        LOG(WARNING) << "MultiGetCommand OnError id=" << id();
        mget_result_.code = err_code;
		auto & result_map = mget_result_.get_result_map_;
        for (auto it = keys_.begin(); it != keys_.end(); ++it) {
			if (result_map.find(*it) == result_map.end()) {
				result_map.emplace(*it, GetResult(err_code, "not enough connections"));
			}
>>>>>>> 61398403e90aae391a3b01edbbe836470232be68
		}
		const uint32_t finish = get_handler()->FinishedOne();
		if ((finish + 1) >= get_handler()->vbucket_size()) {
			caller_loop()->RunInLoop(std::bind(get_handler()->get_callback(), std::move(get_handler()->get_result())));
		}
	}
    virtual void OnMultiGetCommandDone(int resp_code, std::string& key, std::string& value);
    virtual void OnMultiGetCommandOneResponse(int resp_code, std::string& key, std::string& value);
    //std::vector<std::string> keys_;
private:
    virtual BufferPtr RequestBuffer();
};

typedef std::shared_ptr<MultiKeyHandler<PrefixMultiGetResult, PrefixMultiGetCallback>> PrefixMultiKeyGetHandlerPtr;

class PrefixMultiGetCommand  : public Command, public MultiKeyMode<PrefixMultiGetResult, PrefixMultiGetCallback> {
public:
    PrefixMultiGetCommand(evpp::EventLoop* evloop, uint16_t vbucket, const PrefixMultiKeyGetHandlerPtr& handler)
        : Command(evloop, vbucket), MultiKeyMode(handler) {
    }

    virtual void OnError(int err_code) {
        LOG(WARNING) << "prefixMultiGetCommand OnError id=" << id();
		auto & keys = get_handler()->FindKeysByid(vbucket_id());
		auto & result_map = get_handler()->get_result().get_result_map_;
		auto k = result_map.begin();
		for (auto it : keys) {
			k = result_map.find(it);
			assert(k != result_map.end());
			k->second->code = err_code;
		}
		const uint32_t finish = get_handler()->FinishedOne();
		if ((finish + 1) >= get_handler()->vbucket_size()) {
			caller_loop()->RunInLoop(std::bind(get_handler()->get_callback(), get_handler()->get_result()));
		}
    }
	virtual PrefixGetResultPtr& GetResultContainerByKey(const std::string& key);
    virtual void OnPrefixGetCommandDone();
private:
    virtual BufferPtr RequestBuffer();
};

class RemoveCommand  : public Command {
public:
    RemoveCommand(evpp::EventLoop* evloop, uint16_t vbucket, const std::string& key, RemoveCallback callback)
        : Command(evloop, vbucket), key_(key), remove_callback_(callback) {
    }
    virtual void OnError(int err_code) {
        LOG(WARNING) << "RemoveCommand OnError id=" << id();

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
    static std::atomic_int next_thread_;
private:
    virtual BufferPtr RequestBuffer();
};

}

