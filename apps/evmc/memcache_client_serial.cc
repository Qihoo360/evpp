#include "memcache_client_serial.h"
#include "evpp/tcp_conn.h"
#include "evpp/timestamp.h"

namespace evmc {

MemcacheClientSerial::~MemcacheClientSerial() {
}

void MemcacheClientSerial::Stop() {
    MemcacheClientBase::Stop();
}

void MemcacheClientSerial::Set(const std::string& key, const std::string& value, uint32_t flags,
                               uint32_t expire, SetCallback callback) {
    CommandPtr command = std::make_shared<SetCommand>(loop_, 0, key, value, flags, expire, callback);
    command->set_id(next_id());
    LaunchCommand(command);
}

void MemcacheClientSerial::Remove(const std::string& key, RemoveCallback callback) {
    CommandPtr command = std::make_shared<RemoveCommand>(loop_, 0, key, callback);
    command->set_id(next_id());
    LaunchCommand(command);
}

void MemcacheClientSerial::Get(const std::string& key, GetCallback callback) {
    CommandPtr command = std::make_shared<GetCommand>(loop_, 0, key, callback);
    command->set_id(next_id());
    LaunchCommand(command);
}

void MemcacheClientSerial::MultiGet(const std::vector<std::string>& keys, MultiGetCallback callback) {
    if (UNLIKELY(keys.size() <= 0)) {
        MultiGetResult result;
        callback(std::move(result));
        return;
    }
    CommandPtr command = std::make_shared<SerialMultiGetCommand>(loop_, callback);
    command->set_id(next_id());
    (static_cast<SerialMultiGetCommand*>(command.get()))->PacketRequests(keys);
    LaunchCommand(command);
}

bool MemcacheClientSerial::Start(evpp::EventLoop* loop) {
    if (!loop) {
        LOG_ERROR << "start with nullptr event loop";
        return false;
    }
    MemcacheClientBase::Start(false);
    std::map<std::string, MemcacheClientPtr> client_map;
    loop_ = loop;
    MemcacheClientBase::BuilderMemClient(loop_, server_, client_map, timeout_ms_);
    assert(client_map.find(server_) != client_map.end());
    memclient_ = client_map[server_];
    return true;
}

void MemcacheClientSerial::LaunchCommand(CommandPtr& command) {
    auto conn = memclient_->conn();
    assert(memclient_);
    //不需要重试
    command->set_server_id(0);
    command->set_server_id(0);
    if (LIKELY(conn && conn->IsConnected())) {
        memclient_->PushRunningCommand(command);
        command->Launch(memclient_);
        return;
    }
    if (!conn || conn->status() == evpp::TCPConn::kConnecting) {
        memclient_->PushWaitingCommand(command);
    } else {
        //assert(loop_->IsInLoopThread());
        LOG_ERROR << "connected to server, but some problems occurs!";
        command->OnError(ERR_CODE_NETWORK);
    }
}

}

