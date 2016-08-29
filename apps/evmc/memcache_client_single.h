/*************************************************************************
	> File Name: memcache_client_single.h
	> Author: ma6174
	> Mail: ma6174@163.com 
	> Created Time: Sun 28 Aug 2016 04:17:36 PM CST
 ************************************************************************/
#ifndef MEMCACHE_CLIENT_SINGLE_H_
#define MEMCACHE_CLIENT_SINGLE_H_

#include "mctypes.h"
#include "vbucket_config.h"
#include <queue>
namespace evmc {
class CommandSingle {
	public:
		CommandSingle(evpp::EventLoop* caller_loop, const MultiGetCallback2& cb, const uint32_t thread_id):
			caller_loop_(caller_loop), kvs_(std::make_shared<MultiGetMapResult>()), callback_(cb), id_(thread_id){
		}

		void Collect(std::string & key, GetResult & result, bool is_done) {

			kvs_->insert(std::make_pair(std::move(key), std::move(result)));

			if (is_done) {
				if (caller_loop_ && !caller_loop_->IsInLoopThread()) {
					caller_loop_->RunInLoop(std::bind(callback_, kvs_, 0));
				} else {
					callback_(kvs_, 0);
				}
			}
		}

	void Error (int errorcode) {
		if (caller_loop_ && !caller_loop_->IsInLoopThread()) {
			caller_loop_->RunInLoop(std::bind(callback_, kvs_, errorcode));
		} else {
			callback_(kvs_, errorcode);
		}
	}
	inline std::string & buf() {
		return buf_;
	}
	inline uint32_t id() {
		return id_;
	}
	private:
	evpp::EventLoop* caller_loop_;
	MultiGetMapResultPtr kvs_;
    MultiGetCallback2 callback_;
	std::string buf_;
	uint32_t id_;
};
typedef std::shared_ptr<CommandSingle> CommandSinglePtr;

class MemcacheClientSingle {

	public:
		MemcacheClientSingle(const char* vbucket_conf, int thread_num, int timeout_ms)
			: vbucket_conf_file_(vbucket_conf), timeout_(timeout_ms / 1000.0), loop_(NULL),
		tcp_client_(NULL), thread_id_(0) {
			}

		~MemcacheClientSingle();
		void Stop(bool wait_thread_exit);
		bool Start(evpp::EventLoop * loop);
		void MultiGet2(evpp::EventLoop* caller_loop, const std::vector<std::string>& keys, MultiGetCallback2 callback);
	private:
		CommandSinglePtr PeekRunningCommand();
		inline void PushRunningCommand(CommandSinglePtr& cmd) ;
		inline void PopRunningCommand();
		inline void PushWaitingCommand(CommandSinglePtr& cmd);
		CommandSinglePtr PeekPopWaitingCommand();
		void OnClientConnection(const evpp::TCPConnPtr& conn);
		void OnResponse(const evpp::TCPConnPtr& conn,
                                 evpp::Buffer* buf,
                                 evpp::Timestamp ts);
		void OnPacketTimeout(uint32_t cmd_id);
		void Packet(const std::vector<std::string> & keys, std::string& data, const uint32_t id);
		void SendData(CommandSinglePtr& command);
	private:
		std::string vbucket_conf_file_;
		evpp::Duration timeout_;
		evpp::EventLoop * loop_;
		evpp::TCPClient * tcp_client_;
		uint32_t thread_id_;
		evpp::InvokeTimerPtr cmd_timer_;
		std::queue<CommandSinglePtr> running_command_;
		std::queue<CommandSinglePtr> waiting_command_;

};
}
#endif
