#include "evpp/exp.h"
// #include "test_common.h"

#include "evpp/libevent_headers.h"
#include "evpp/libevent_watcher.h"
#include "evpp/event_loop.h"
#include "evpp/event_loop_thread.h"
#include "evpp/buffer.h"
#include "evpp/tcp_conn.h"
#include "evpp/tcp_client.h"

#include <thread>
#include <queue>
#include <memcached/protocol_binary.h>

namespace {

// TODO 
// 1. pipeline
// 2. error handling
// 3. set/remove/mget
// 4. server addr hash
// 5. embeded & standalone

typedef std::shared_ptr<evpp::Buffer> BufferPtr;
typedef std::shared_ptr<evpp::EventLoop> EventLoopPtr;

struct GetResult {
    GetResult() : code(-1) {}
    GetResult(int c, const std::string& v) : code(c), value(v) {}
    int code;
    std::string value;
};

typedef std::map<std::string, GetResult> MultiGetResult;

typedef std::function<void(const std::string key, GetResult result)> GetCallback;
typedef std::function<void(const std::string key, int code)> SetCallback;
typedef std::function<void(const std::string key, int code)> RemoveCallback;

typedef std::function<void(MultiGetResult result)> MultiGetCallback;

class MemcacheClient;
class Command  : public std::enable_shared_from_this<Command> {
  public:
    Command(EventLoopPtr ev_loop, const char* key) : ev_loop_(ev_loop), key_(key) {
    }
    virtual BufferPtr RequestBuffer() const = 0;

    EventLoopPtr ev_loop_;
    std::string key_;

    // TODO : mget_result_放到子类中
    MultiGetResult mget_result_;

    GetCallback get_callback_;
    GetCallback getq_callback_;
    MultiGetCallback mget_callback_;
    SetCallback set_callback_;
    RemoveCallback remove_callback_;

    std::string ServerAddr() const {
        // const static std::string test_addr = "127.0.0.1:11611";
        const static std::string test_addr = "10.160.112.97:10002";
        return test_addr;
    }

    void Launch(MemcacheClient * memc_client);

    void GetqCallback(const std::string key, GetResult res) {
        // TODO : 检查确保始终在tcp_client线程中执行
        LOG_INFO << "GetqCallback " << key << " " << res.code << " " << res.value;
        mget_result_[key] = res;
    }
};


typedef std::shared_ptr<Command> CommandPtr;

class SetCommand  : public Command {
  public:
    SetCommand(EventLoopPtr ev_loop, const char* key, const char * value, size_t val_len, uint32_t flags, uint32_t expire)
           : Command(ev_loop, key), value_(value, val_len), flags_(flags), expire_(expire) {}
    std::string value_;
    uint32_t flags_;
    uint32_t expire_;

    virtual BufferPtr RequestBuffer() const {
        protocol_binary_request_header req;
        bzero((void*)&req, sizeof(req));

        req.request.magic  = PROTOCOL_BINARY_REQ;
        req.request.opcode = PROTOCOL_BINARY_CMD_SET;
        req.request.keylen = htons(key_.size());
        req.request.extlen = 8;
        req.request.datatype = PROTOCOL_BINARY_RAW_BYTES;
        size_t bodylen = req.request.extlen + key_.size() + value_.size();
        req.request.bodylen = htonl(static_cast<uint32_t>(bodylen));

        BufferPtr buf(new evpp::Buffer(sizeof(protocol_binary_request_header) + key_.size()));
        buf->Append((void*)&req, sizeof(req));
        buf->AppendInt32(flags_);
        buf->AppendInt32(expire_);
        buf->Append(key_.data(), key_.size());
        buf->Append(value_.data(), value_.size());

        return buf;
    }
};

class GetCommand  : public Command {
  public:
    GetCommand(EventLoopPtr ev_loop, const char* key) : Command(ev_loop, key) {}

    virtual BufferPtr RequestBuffer() const {
        protocol_binary_request_header req;
        bzero((void*)&req, sizeof(req));

        req.request.magic  = PROTOCOL_BINARY_REQ;
        req.request.opcode = PROTOCOL_BINARY_CMD_GETQ;
        req.request.keylen = htons(key_.size());
        req.request.datatype = PROTOCOL_BINARY_RAW_BYTES;
        req.request.bodylen = htonl(key_.size());

        BufferPtr buf(new evpp::Buffer(sizeof(protocol_binary_request_header) + key_.size()));
        buf->Append((void*)&req, sizeof(req));
        buf->Append(key_.data(), key_.size());

        return buf;
    }
};


class MultiGetCommand  : public Command {
  public:
    MultiGetCommand(EventLoopPtr ev_loop, const std::vector<std::string>& keys)
            : Command(ev_loop, ""), keys_(keys) {
    }
    std::vector<std::string> keys_;

    virtual BufferPtr RequestBuffer() const {
        BufferPtr buf(new evpp::Buffer(50 * keys_.size()));  // 预分配长度多数情况够长

        for(size_t i = 0; i < keys_.size(); ++i) {
            protocol_binary_request_header req;
            bzero((void*)&req, sizeof(req));
            req.request.magic    = PROTOCOL_BINARY_REQ;
            if (i < keys_.size() - 1) {
                req.request.opcode   = PROTOCOL_BINARY_CMD_GETQ;
            } else {
                req.request.opcode   = PROTOCOL_BINARY_CMD_GET;
            }
            req.request.keylen   = htons(keys_[i].size());
            req.request.datatype = PROTOCOL_BINARY_RAW_BYTES;
            req.request.bodylen  = htonl(keys_[i].size());

            buf->Append((void*)&req, sizeof(req));
            buf->Append(keys_[i].data(), keys_[i].size());
        }

        return buf;
    }
};

class RemoveCommand  : public Command {
  public:
    RemoveCommand(EventLoopPtr ev_loop, const char* key)
           : Command(ev_loop, key) {}

    virtual BufferPtr RequestBuffer() const {
        protocol_binary_request_header req;
        bzero((void*)&req, sizeof(req));

        req.request.magic  = PROTOCOL_BINARY_REQ;
        req.request.opcode = PROTOCOL_BINARY_CMD_DELETE;
        req.request.keylen = htons(key_.size());
        req.request.datatype = PROTOCOL_BINARY_RAW_BYTES;
        req.request.bodylen = htonl(static_cast<uint32_t>(key_.size()));

        BufferPtr buf(new evpp::Buffer(sizeof(protocol_binary_request_header) + key_.size()));
        buf->Append((void*)&req, sizeof(req));
        buf->Append(key_.data(), key_.size());

        return buf;
    }
};


class BinaryCodec
{
public:
    BinaryCodec(MemcacheClient * memc_client) : memc_client_(memc_client) {}

    void OnCodecMessage(const evpp::TCPConnPtr& conn,
           evpp::Buffer* buf,
           evpp::Timestamp ts)
    {
        while (buf->size() >= kHeaderLen) // kHeaderLen == 24
        {
            const void* data = buf->data();
            protocol_binary_response_header resp = 
                *static_cast<const protocol_binary_response_header*>(data);
            resp.response.bodylen = ntohl(resp.response.bodylen);
            resp.response.status  = ntohs(resp.response.status);
            resp.response.keylen  = ntohs(resp.response.keylen);
            if (resp.response.bodylen < 0) {
                LOG_ERROR << "Invalid length " << resp.response.bodylen;
                conn->Close();
                break;
            } else if (buf->size() >= resp.response.bodylen + kHeaderLen) {
                OnResponsePacket(resp, buf);
            } else {
                LOG_TRACE << "need recv more data";
                break;
            }
        }
    }

private:
    // noncopyable
    BinaryCodec(const BinaryCodec&);
    const BinaryCodec& operator=(const BinaryCodec&);

    void OnResponsePacket(const protocol_binary_response_header& resp,
                evpp::Buffer* buf);
private:
    MemcacheClient * memc_client_;

    static const size_t kHeaderLen = sizeof(protocol_binary_response_header);
};

class MemcacheClient {
  public:
    MemcacheClient(evpp::TCPClient * tcp_client) : tcp_client_(tcp_client) {}

    std::queue<CommandPtr> command_waiting_;
    std::queue<CommandPtr> command_running_;
    evpp::TCPConnPtr conn() const { return tcp_client_->conn();}

    void EmbededAsyncGet(const char* key, GetCallback callback) {
        CommandPtr command(new GetCommand(EventLoopPtr(), key));
        command->get_callback_ = callback;
        command->Launch(this);
    }

    void OnSetCommandDone(uint32_t command_id, int memcached_response_code);
    void OnRemoveCommandDone(uint32_t command_id, int memcached_response_code);
    void OnGetCommandDone(uint32_t command_id, int memcached_response_code, 
            const std::string& return_value);
    void OnMultiGetCommandOneResponse(uint32_t command_id, 
            int memcached_response_code, const std::string& return_value);
    void OnMultiGetCommandDone(uint32_t noop_cmd_id, int memcached_response_code, const std::string& return_value);
  private:
    evpp::TCPClient * tcp_client_;
    // noncopyable
    MemcacheClient(const MemcacheClient&);
    const MemcacheClient& operator=(const MemcacheClient&);
};

void MemcacheClient::OnSetCommandDone(uint32_t command_id, int memcached_response_code) {
    CommandPtr command(command_running_.front());
    command_running_.pop();
    LOG_INFO << "OnSetCommandDone id=" << command_id << " code=" << memcached_response_code;
    if (command->ev_loop_) {
        command->ev_loop_->RunInLoop(std::bind(command->set_callback_,
                               command->key_, memcached_response_code));
    } else {
        command->set_callback_(command->key_, memcached_response_code);
    }
}

void MemcacheClient::OnRemoveCommandDone(uint32_t command_id, int memcached_response_code) {
    CommandPtr command(command_running_.front());
    command_running_.pop();
    LOG_INFO << "OnRemoveCommandDone id=" << command_id << " code=" << memcached_response_code;
    if (command->ev_loop_) {
        command->ev_loop_->RunInLoop(std::bind(command->remove_callback_,
                               command->key_, memcached_response_code));
    } else {
        command->remove_callback_(command->key_, memcached_response_code);
    }
}

void MemcacheClient::OnGetCommandDone(uint32_t command_id, int memcached_response_code, 
            const std::string& return_value) {
    CommandPtr command(command_running_.front());
    command_running_.pop();
    LOG_INFO << "OnGetCommandDone id=" << command_id << " code=" << memcached_response_code;
    if (command->ev_loop_) {
        command->ev_loop_->RunInLoop(std::bind(command->get_callback_, command->key_,
                               GetResult(memcached_response_code, return_value)));
    } else {
        command->get_callback_(command->key_, GetResult(memcached_response_code, return_value));
    }
}

void MemcacheClient::OnMultiGetCommandOneResponse(uint32_t command_id, 
            int memcached_response_code, const std::string& return_value) {
    CommandPtr command(command_running_.front());
    // command_running_.pop();

    LOG_INFO << "OnMultiGetCommandOneResponse id=" << command_id << " code=" << memcached_response_code;
    // 注意，该回调不直接返回给用户，且涉及mget_command写操作，必须一律在当前线程中执行
    command->getq_callback_(command->key_, GetResult(memcached_response_code, return_value));
}

void MemcacheClient::OnMultiGetCommandDone(uint32_t command_id, 
            int memcached_response_code, const std::string& return_value) {
    CommandPtr command(command_running_.front());
    command_running_.pop();

    LOG_INFO << "OnMultiGetCommandOneResponse id=" << command_id << " code=" << memcached_response_code;
    // 注意，该回调不直接返回给用户，且涉及mget_command写操作，必须一律在当前线程中执行
    command->getq_callback_(command->key_, GetResult(memcached_response_code, return_value));

    if (command->ev_loop_) {
        command->ev_loop_->RunInLoop(std::bind(command->mget_callback_, command->mget_result_));
    } else {
        command->mget_callback_(command->mget_result_);
    }
}

/*
void MemcacheClient::OnGetCommandDone(uint32_t command_id, int memcached_response_code, 
            const std::string& return_value) {
    CommandPtr command(command_running_.front());
    command_running_.pop();
    LOG_INFO << "OnGetCommandDone id=" << command_id << " code=" << memcached_response_code;
    if (command->ev_loop_) {
        command->ev_loop_->RunInLoop(std::bind(command->memcached_response_code);
    }
}

void MemcacheClient::OnGetCommandDone(uint32_t command_id, int memcached_response_code, 
            const std::string& return_value) {
    CommandPtr command(command_running_.front());
    command_running_.pop();
    LOG_INFO << "OnGetCommandDone id=" << command_id << " code=" << memcached_response_code;
    if (command->ev_loop_) {
        command->ev_loop_->RunInLoop(std::bind(command->, memcached_response_code, return_value));
    } else {
        command->get_callback_(memcached_response_code, return_value);
    }
}
*/

void BinaryCodec::OnResponsePacket(const protocol_binary_response_header& resp,
            evpp::Buffer* buf) {
    uint32_t id     = resp.response.opaque;  // no need to call ntohl
    int      opcode = resp.response.opcode;
    switch (opcode) {
        case PROTOCOL_BINARY_CMD_SET:
            memc_client_->OnSetCommandDone(id, resp.response.status);
            break;
        case PROTOCOL_BINARY_CMD_DELETE:
            memc_client_->OnRemoveCommandDone(id, resp.response.status);
            break;
        case PROTOCOL_BINARY_CMD_GET:
            {
                const char* pv = buf->data() + sizeof(resp) + resp.response.extlen;
                std::string value(pv, resp.response.bodylen - resp.response.extlen);
                if (id > 0) {
                    memc_client_->OnGetCommandDone(id, resp.response.status, value);
                } else {
                    memc_client_->OnMultiGetCommandDone(id, resp.response.status, value);
                }
            }
            break;
        case PROTOCOL_BINARY_CMD_GETQ:
            {
                const char* pv = buf->data() + sizeof(resp) + resp.response.extlen;
                std::string value(pv, resp.response.bodylen - resp.response.extlen);
                memc_client_->OnMultiGetCommandOneResponse(id, resp.response.status, value);
                //LOG_DEBUG << "GETQ, opaque=" << id << " value=" << value;
            }
            break;
        case PROTOCOL_BINARY_CMD_NOOP:
          ////LOG_DEBUG << "GETQ, NOOP opaque=" << id;
          //memc_client_->onMultiGetCommandDone(id, resp.response.status);
          //break;
        default:
            break;
    }
    buf->Retrieve(kHeaderLen + resp.response.bodylen);
}

void Command::Launch(MemcacheClient * memc_client) {
    if (!memc_client->conn()) {
        LOG_ERROR << "Command bad memc_client " << memc_client << " key=" << key_;
        return;
    }
    BufferPtr buf = RequestBuffer();
    memc_client->conn()->Send(buf->data(), buf->size());
}

class MemcacheClientPool {
  public:
    MemcacheClientPool(int conncurrency) : loop_pool_(&loop_, conncurrency) {
        loop_pool_.Start(true);
    }
    virtual ~MemcacheClientPool() {
        loop_pool_.Stop(true);
    }

    void AsyncSet(EventLoopPtr ev_loop, const std::string& key, const std::string& value, SetCallback callback) {
        AsyncSet(ev_loop, key.c_str(), value.c_str(), value.size(), 0, 0, callback);
    }
    void AsyncSet(EventLoopPtr ev_loop, const char* key, const char* value, size_t val_len, uint32_t flags,
                  uint32_t expire, SetCallback callback) {
        CommandPtr command(new SetCommand(ev_loop, key, value, val_len , flags, expire));
        command->set_callback_ = callback;

        ev_loop->RunInLoop(std::bind(&MemcacheClientPool::LaunchCommand, this, command));
    }

    void AsyncRemove(EventLoopPtr ev_loop, const char* key, RemoveCallback callback) {
        CommandPtr command(new RemoveCommand(ev_loop, key));
        command->remove_callback_ = callback;

        ev_loop->RunInLoop(std::bind(&MemcacheClientPool::LaunchCommand, this, command));
    }

    void AsyncGet(EventLoopPtr ev_loop, const char* key, GetCallback callback) {
        CommandPtr command(new GetCommand(ev_loop, key));
        command->get_callback_ = callback;

        ev_loop->RunInLoop(std::bind(&MemcacheClientPool::LaunchCommand, this, command));
    }

    void AsyncMultiGet(EventLoopPtr ev_loop, const std::vector<std::string>& keys, MultiGetCallback callback) {
        LOG_INFO << "AsyncMultiGet keys=" << keys.size();
        CommandPtr command(new MultiGetCommand(ev_loop, keys));
        command->mget_callback_ = callback;
        command->getq_callback_ = std::bind(&Command::GetqCallback, command, 
                               std::placeholders::_1, std::placeholders::_2);
        ev_loop->RunInLoop(std::bind(&MemcacheClientPool::LaunchCommand, this, command));
    }
  private:
    // noncopyable
    MemcacheClientPool(const MemcacheClientPool&);
    const MemcacheClientPool& operator=(const MemcacheClientPool&);

    static void OnClientConnection(const evpp::TCPConnPtr& conn, MemcacheClient* memc_client) {
        LOG_INFO << "OnClientConnection conn=" << conn.get() << " memc_conn=" << memc_client->conn().get();
        if (conn->IsConnected()) {
            LOG_INFO << "OnClientConnection connect ok";
            while(!memc_client->command_waiting_.empty()) {
                CommandPtr command(memc_client->command_waiting_.front());
                memc_client->command_waiting_.pop();

                memc_client->command_running_.push(command);
                command->Launch(memc_client);
            }
        } else {
            LOG_INFO << "Disconnected from " << conn->remote_addr();
        }
    }

    void LaunchCommand(CommandPtr command) {
        LOG_INFO << "LaunchCommand";
        loop_pool_.GetNextLoopWithHash(100)->RunInLoop(std::bind(&MemcacheClientPool::DoLaunchCommand, this, command));
    }

  private:
    void DoLaunchCommand(CommandPtr command) {
        std::map<std::string, MemcacheClient *>::iterator it = memc_clients_.find(command->ServerAddr());

        if (it == memc_clients_.end()) {
            evpp::TCPClient * tcp_client = new evpp::TCPClient(loop_pool_.GetNextLoopWithHash(100), command->ServerAddr(), "MemcacheBinaryClient");
            MemcacheClient * memc_client = new MemcacheClient(tcp_client);
            LOG_INFO << "DoLaunchCommand new tcp_client=" << tcp_client << " memc_client=" << memc_client;

            BinaryCodec* codec = new BinaryCodec(memc_client);

            tcp_client->SetConnectionCallback(std::bind(&MemcacheClientPool::OnClientConnection, std::placeholders::_1, memc_client));
            tcp_client->SetMesageHandler(std::bind(&BinaryCodec::OnCodecMessage, codec, std::placeholders::_1,
                                         std::placeholders::_2, std::placeholders::_3));
            tcp_client->Connect();

            memc_clients_[command->ServerAddr()] = memc_client;

            memc_client->command_waiting_.push(command);
            return;
        }
        if (!it->second->conn() || !it->second->conn()->IsConnected()) {
            it->second->command_waiting_.push(command);
            return;
        }
        it->second->command_running_.push(command);
        command->Launch(it->second);
    }

    thread_local static std::map<std::string, MemcacheClient *> memc_clients_;
    evpp::EventLoop loop_;
    evpp::EventLoopThreadPool loop_pool_;
};

thread_local std::map<std::string, MemcacheClient *> MemcacheClientPool::memc_clients_;

static void OnTestSetDone(const std::string key, int code) {
    LOG_INFO << "OnTestSetDone " << code << " " << key;
}
static void OnTestGetDone(const std::string key, GetResult res) {
    LOG_INFO << "OnTestGetDone " << key << " " << res.code << " " << res.value;
}
static void OnTestRemoveDone(const std::string key, int code) {
    LOG_INFO << "OnTestRemoveDone " << code << " " << key;
}
static void OnTestMultiGetDone(MultiGetResult res) {
    MultiGetResult::const_iterator it = res.begin();
    for(; it != res.end(); ++it) {
        LOG_INFO << "OnTestGetDone " << it->first << " " << it->second.code << " " << it->second.value;
    }
}

}

namespace evloop
{
    static EventLoopPtr g_loop;
    static void StopLoop() {
        g_loop->Stop();
    }

    static void MyEventThread() {
        LOG_INFO << "EventLoop is running ...";
        g_loop = EventLoopPtr(new evpp::EventLoop);
        g_loop->Run();
    }
}

int main() {
// TEST_UNIT(testMemcacheClient) {
    std::thread th(evloop::MyEventThread);
    MemcacheClientPool mcp(16);

    const static int MAX_KEY = 10;

    std::vector<std::string> mget_keys;
    for(size_t i = 0; i < MAX_KEY; ++i) {
        std::stringstream ss;
        ss << "test" << i;
        mget_keys.push_back(ss.str());
    }
    mcp.AsyncMultiGet(evloop::g_loop, mget_keys, &OnTestMultiGetDone);

#if 0

    for(size_t i = 0; i < MAX_KEY; i += 2) {
        std::stringstream ss_key;
        ss_key << "test" << i;
        std::stringstream ss_value;
        ss_value << "test_value" << i;
        mcp.AsyncSet(evloop::g_loop, ss_key.str(), ss_value.str(), &OnTestSetDone);
    }
    for(size_t i = 0; i < MAX_KEY; ++i) {
        std::stringstream ss;
        ss << "test" << i;
        mcp.AsyncGet(evloop::g_loop, ss.str().c_str(), &OnTestGetDone);
    }
    for(size_t i = 0; i < MAX_KEY/2; ++i) {
        std::stringstream ss_key;
        ss_key << "test" << i;
        mcp.AsyncRemove(evloop::g_loop, ss_key.str().c_str(), &OnTestRemoveDone);
    }
#endif

    evloop::g_loop->RunAfter(2000 * 1000.0, &evloop::StopLoop);
    th.join();
    // usleep(100*1000);
    return 0;
}

