#include "evpp/exp.h"
#include "test_common.h"

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
// - error handling: conn, timeout
// - server addr hash / vbucket
// - embeded & standalone

typedef std::shared_ptr<evpp::Buffer> BufferPtr;
typedef std::shared_ptr<evpp::EventLoop> EventLoopPtr;
typedef std::shared_ptr<evpp::TCPClient> TcpClientPtr;
typedef std::shared_ptr<evpp::TimerEventWatcher> TimerEventPtr;

const int ERR_CODE_TIMEOUT = -1;
const int ERR_CODE_NETWORK = -2;

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

typedef std::function<void(const std::string key, GetResult result)> GetCallback;
typedef std::function<void(const std::string key, int code)> SetCallback;
typedef std::function<void(const std::string key, int code)> RemoveCallback;
typedef std::function<void(MultiGetResult result)> MultiGetCallback;

/*
int vbucket_get_vbucket_by_key(VBUCKET_CONFIG_HANDLE vb, const void *key, size_t nkey) {
    uint32_t digest = libhashkit_digest(key, nkey, vb->hk_algorithm);
    return digest % vb->num_vbuckets;
    //return digest & vb->mask;
}


uint32_t libhashkit_digest(const char *key, size_t key_length, hashkit_hash_algorithm_t hash_algorithm)
{
  switch (hash_algorithm)
  {
  case HASHKIT_HASH_DEFAULT:
    return libhashkit_one_at_a_time(key, key_length);
  case HASHKIT_HASH_MD5:
    return libhashkit_md5(key, key_length);
  case HASHKIT_HASH_CRC:
    return libhashkit_crc32(key, key_length);
  case HASHKIT_HASH_FNV1_64:
    return libhashkit_fnv1_64(key, key_length);
  case HASHKIT_HASH_FNV1A_64:
    return libhashkit_fnv1a_64(key, key_length);
  case HASHKIT_HASH_FNV1_32:
    return libhashkit_fnv1_32(key, key_length);
  case HASHKIT_HASH_FNV1A_32:
    return libhashkit_fnv1a_32(key, key_length);
  case HASHKIT_HASH_HSIEH:
#ifdef HAVE_HSIEH_HASH
    return libhashkit_hsieh(key, key_length);
#else
    return 1;
#endif
  case HASHKIT_HASH_MURMUR:
    return libhashkit_murmur(key, key_length);
  case HASHKIT_HASH_JENKINS:
    return libhashkit_jenkins(key, key_length);
  case HASHKIT_HASH_CUSTOM:
  case HASHKIT_HASH_MAX:
  default:
#ifdef HAVE_DEBUG
    fprintf(stderr, "hashkit_hash_t was extended but libhashkit_generate_value was not updated\n");
    fflush(stderr);
    assert(0);
#endif
    break;
  }

  return 1;
}


uint32_t libhashkit_md5(const char *key, size_t key_length)
{
  return hashkit_md5(key, key_length, NULL);
}

uint32_t hashkit_md5(const char *key, size_t key_length, void *context __attribute__((unused)))
{
  unsigned char results[16];

  md5_signature((unsigned char*)key, (unsigned int)key_length, results); // 这一步计算MD5

  return ((uint32_t) (results[3] & 0xFF) << 24)
    | ((uint32_t) (results[2] & 0xFF) << 16)
    | ((uint32_t) (results[1] & 0xFF) << 8)
    | (results[0] & 0xFF);
}
*/

class MemcacheClient;
class Command  : public std::enable_shared_from_this<Command> {
public:
    Command(EventLoopPtr evloop) : caller_loop_(evloop), id_(0), thread_hash_(rand()) {
        vbucket_id_ = 0;
    }

    uint32_t id() const { return id_; }
    uint32_t thread_hash() const { return thread_hash_; }
    uint16_t vbucket_id() const { return vbucket_id_; }
    EventLoopPtr caller_loop() const { return caller_loop_; }

    std::string ServerAddr() const {
        // const static std::string test_addr = "127.0.0.1:11611";
        const static std::string test_addr = "10.160.112.97:10002";
        return test_addr;
    }

    void Launch(MemcacheClient* memc_client);

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
    uint32_t thread_hash_;
    uint16_t vbucket_id_;
};


typedef std::shared_ptr<Command> CommandPtr;

class SetCommand  : public Command {
public:
    SetCommand(EventLoopPtr evloop, const char* key, const char * value, size_t val_len,
               uint32_t flags, uint32_t expire, SetCallback callback)
           : Command(evloop), key_(key), value_(value, val_len),
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
    virtual BufferPtr RequestBuffer() const {
        protocol_binary_request_header req;
        bzero((void*)&req, sizeof(req));

        req.request.magic  = PROTOCOL_BINARY_REQ;
        req.request.opcode = PROTOCOL_BINARY_CMD_SET;
        req.request.keylen = htons(key_.size());
        req.request.extlen = 8;
        req.request.datatype = PROTOCOL_BINARY_RAW_BYTES;
        req.request.vbucket  = vbucket_id();
        req.request.opaque   = id();
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
    GetCommand(EventLoopPtr evloop, const char* key, GetCallback callback) 
            : Command(evloop)
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
    virtual BufferPtr RequestBuffer() const {
        protocol_binary_request_header req;
        bzero((void*)&req, sizeof(req));

        req.request.magic  = PROTOCOL_BINARY_REQ;
        req.request.opcode = PROTOCOL_BINARY_CMD_GET;
        req.request.keylen = htons(key_.size());
        req.request.datatype = PROTOCOL_BINARY_RAW_BYTES;
        req.request.vbucket  = vbucket_id();
        req.request.opaque   = id();
        req.request.bodylen = htonl(key_.size());

        BufferPtr buf(new evpp::Buffer(sizeof(protocol_binary_request_header) + key_.size()));
        buf->Append((void*)&req, sizeof(req));
        buf->Append(key_.data(), key_.size());

        return buf;
    }
};


class MultiGetCommand  : public Command {
public:
    MultiGetCommand(EventLoopPtr evloop, const std::vector<std::string>& keys, MultiGetCallback callback)
            : Command(evloop), keys_(keys), mget_callback_(callback) {
    }

    virtual void OnError(int err_code) {
        mget_result_.code = err_code;
        if (caller_loop()) {
            caller_loop()->RunInLoop(std::bind(mget_callback_, mget_result_));
        } else {
            mget_callback_(mget_result_);
        }
    }
    virtual void OnMultiGetCommandDone(int resp_code, const std::string& key, const std::string& value) {
        if (resp_code == PROTOCOL_BINARY_RESPONSE_SUCCESS) {
            mget_result_.get_result_map_[key] = GetResult(resp_code, value);
        }

        if (caller_loop()) {
            caller_loop()->RunInLoop(std::bind(mget_callback_, mget_result_));
        } else {
            mget_callback_(mget_result_);
        }
    }
    virtual void OnMultiGetCommandOneResponse(int resp_code, const std::string& key, const std::string& value) {
        LOG_INFO << "OnMultiGetCommandOneResponse " << key << " " << resp_code << " " << value;
        if (resp_code == PROTOCOL_BINARY_RESPONSE_SUCCESS) {
            mget_result_.get_result_map_[key] = GetResult(resp_code, value);
        }
        mget_result_.code = resp_code;
    }
private:
    std::vector<std::string> keys_;
    MultiGetCallback mget_callback_;
    MultiGetResult mget_result_;
private:
    virtual BufferPtr RequestBuffer() const {
        BufferPtr buf(new evpp::Buffer(50 * keys_.size()));  // 预分配长度多数情况够长

        for(size_t i = 0; i < keys_.size(); ++i) {
            protocol_binary_request_header req;
            bzero((void*)&req, sizeof(req));
            req.request.magic    = PROTOCOL_BINARY_REQ;
            if (i < keys_.size() - 1) {
                req.request.opcode   = PROTOCOL_BINARY_CMD_GETKQ;
            } else {
                req.request.opcode   = PROTOCOL_BINARY_CMD_GETK;
            }
            req.request.keylen   = htons(keys_[i].size());
            req.request.datatype = PROTOCOL_BINARY_RAW_BYTES;
            req.request.vbucket  = vbucket_id();
            req.request.opaque   = id();
            req.request.bodylen  = htonl(keys_[i].size());

            buf->Append((void*)&req, sizeof(req));
            buf->Append(keys_[i].data(), keys_[i].size());
        }

        return buf;
    }
};

class RemoveCommand  : public Command {
public:
    RemoveCommand(EventLoopPtr evloop, const char* key, RemoveCallback callback)
           : Command(evloop), key_(key), remove_callback_(callback) {
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
    virtual BufferPtr RequestBuffer() const {
        protocol_binary_request_header req;
        bzero((void*)&req, sizeof(req));

        req.request.magic  = PROTOCOL_BINARY_REQ;
        req.request.opcode = PROTOCOL_BINARY_CMD_DELETE;
        req.request.keylen = htons(key_.size());
        req.request.datatype = PROTOCOL_BINARY_RAW_BYTES;
        req.request.vbucket  = vbucket_id();
        req.request.opaque   = id();
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
    // TODO : 若使用智能指针，要处理循环引用. client的回调中引用了codec
    MemcacheClient * memc_client_;

    static const size_t kHeaderLen = sizeof(protocol_binary_response_header);
};

class MemcacheClient {
public:
    MemcacheClient(evpp::EventLoop * evloop, evpp::TCPClient * tcp_client, int timeout_ms = 300) : id_seq_(0),
        exec_loop_(evloop), tcp_client_(tcp_client), timeout_(timeout_ms / 1000.0) {
    }

    EventLoopPtr exec_loop() const { return exec_loop_; }

    // 注意必须在event线程内执行
    void PushRunningCommand(CommandPtr cmd) {
        if (!cmd) {
            return;
        }
        running_command_.push(cmd);

        // TODO: 权衡 caller_loop_ 是否要用 shared_ptr
        TimerEventPtr timer(new evpp::TimerEventWatcher(exec_loop_.get(), std::bind(&MemcacheClient::OnPacketTimeout, this, cmd->id()), timeout_));
        timer->Init();
        timer->AsyncWait();
        if (cmd_timer_) {
            cmd_timer_->Cancel();
        }
        cmd_timer_ = timer;
    }

    CommandPtr PopRunningCommand() {
        if (running_command_.empty()) {
            return CommandPtr(); 
        }
        CommandPtr command(running_command_.front());
        running_command_.pop();

        if (running_command_.empty()) {
            cmd_timer_->Cancel();
            cmd_timer_.reset();
        }

        return command;
    }
    CommandPtr peek_running_command() {
        if (running_command_.empty()) {
            return CommandPtr(); 
        }
        return CommandPtr(running_command_.front());
    }

    void push_waiting_command(CommandPtr cmd) {
        if (cmd) {
            waiting_command_.push(cmd);
        }
    }
    CommandPtr pop_waiting_command() {
        if (waiting_command_.empty()) {
            return CommandPtr(); 
        }
        CommandPtr command(waiting_command_.front());
        waiting_command_.pop();

        return command;
    }

    evpp::TCPConnPtr conn() const { return tcp_client_->conn();}
    uint32_t next_id() { return ++id_seq_; }

    void EmbededGet(const char* key, GetCallback callback) {
        CommandPtr command(new GetCommand(EventLoopPtr(), key, callback));
        command->Launch(this);
    }

    void OnPacketTimeout(uint32_t cmd_id) {
        while(!running_command_.empty()) {
            CommandPtr cmd(running_command_.front());
            running_command_.pop();

            LOG_WARN << "OnPacketTimeout cmd=" << cmd->id();
            cmd->OnError(ERR_CODE_TIMEOUT);
            if (cmd->id() == cmd_id) { // 不比大小比相等, 以绕过uint溢出的问题
                break;
            }
        }
    }

private:
    // noncopyable
    MemcacheClient(const MemcacheClient&);
    const MemcacheClient& operator=(const MemcacheClient&);
private:
    // std::atomic_uint id_seq_;
    uint32_t id_seq_;

    EventLoopPtr exec_loop_;
    TcpClientPtr tcp_client_;
    evpp::Duration timeout_;

    TimerEventPtr cmd_timer_;

    std::queue<CommandPtr> running_command_;
    std::queue<CommandPtr> waiting_command_;
};

void BinaryCodec::OnResponsePacket(const protocol_binary_response_header& resp,
            evpp::Buffer* buf) {
    uint32_t id  = resp.response.opaque;  // no need to call ntohl
    int      opcode = resp.response.opcode;
    CommandPtr cmd = memc_client_->peek_running_command();
    if (!cmd) {
        // memc_client_->set_last_command_id(id);
        buf->Retrieve(kHeaderLen + resp.response.bodylen);
        LOG_WARN << "OnResponsePacket NULL cmd";
        return;
    }

    switch (opcode) {
        case PROTOCOL_BINARY_CMD_SET:
            cmd = memc_client_->PopRunningCommand();
            // TODO : id 不一致时候，如何处理?
            if (id != cmd->id()) {
                LOG_WARN << "OnResponsePacket SET bad opaque " << id << ", cmd=" << cmd->id();
                break;
            }
            cmd->OnSetCommandDone(resp.response.status);

            LOG_DEBUG << "OnResponsePacket SET, opaque=" << id;
            break;
        case PROTOCOL_BINARY_CMD_DELETE:
            cmd = memc_client_->PopRunningCommand();
            if (id != cmd->id()) {
                LOG_WARN << "OnResponsePacket DELETE bad opaque " << id << ", cmd=" << cmd->id();
                break;
            }
            cmd->OnRemoveCommandDone(resp.response.status);
            LOG_DEBUG << "OnResponsePacket DELETE, opaque=" << id;
            break;
        case PROTOCOL_BINARY_CMD_GET:
            {
                cmd = memc_client_->PopRunningCommand();
                if (id != cmd->id()) {
                    LOG_WARN << "OnResponsePacket GET bad opaque " << id << ", cmd=" << cmd->id();
                    break;
                }
                const char* pv = buf->data() + sizeof(resp) + resp.response.extlen;
                std::string value(pv, resp.response.bodylen - resp.response.extlen);
                cmd->OnGetCommandDone(resp.response.status, value);
                LOG_DEBUG << "OnResponsePacket GET, opaque=" << id;
            }
            break;
        case PROTOCOL_BINARY_CMD_GETK:
            {
                cmd = memc_client_->PopRunningCommand();
                if (id != cmd->id()) {
                    LOG_WARN << "OnResponsePacket GETK bad opaque " << id << ", cmd=" << cmd->id();
                    break;
                }

                const char* pv = buf->data() + sizeof(resp) + resp.response.extlen;
                std::string key(pv, resp.response.keylen);
                std::string value(pv, resp.response.bodylen - resp.response.keylen - resp.response.extlen);

                cmd->OnMultiGetCommandDone(resp.response.status, key, value);
                LOG_DEBUG << "OnResponsePacket GETK, opaque=" << id << " key=" << key;
            }
            break;
        case PROTOCOL_BINARY_CMD_GETKQ:
            {
                cmd = memc_client_->peek_running_command();
                if (id != cmd->id()) {
                    LOG_WARN << "OnResponsePacket GETKQ bad opaque " << id << ", cmd=" << cmd->id();
                    break;
                }

                const char* pv = buf->data() + sizeof(resp) + resp.response.extlen;
                std::string key(pv, resp.response.keylen);
                std::string value(pv, resp.response.bodylen - resp.response.keylen - resp.response.extlen);

                cmd->OnMultiGetCommandOneResponse(resp.response.status, key, value);
                LOG_DEBUG << "OnResponsePacket GETKQ, opaque=" << id << " key=" << key;
            }
            break;
        case PROTOCOL_BINARY_CMD_NOOP:
          ////LOG_DEBUG << "GETQ, NOOP opaque=" << id;
          //memc_client_->onMultiGetCommandDone(id, resp.response.status);
          //break;
        default:
            break;
    }
    // memc_client_->set_last_command_id(id);
    buf->Retrieve(kHeaderLen + resp.response.bodylen);
}

void Command::Launch(MemcacheClient * memc_client) {
    id_ = memc_client->next_id();
    if (!memc_client->conn()) {
        // TODO : 触发网络错误回调
        LOG_ERROR << "Command bad memc_client " << memc_client << " id=" << id_;
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

    void Set(EventLoopPtr caller_loop, const std::string& key, const std::string& value, SetCallback callback) {
        Set(caller_loop, key.c_str(), value.c_str(), value.size(), 0, 0, callback);
    }
    void Set(EventLoopPtr caller_loop, const char* key, const char* value, size_t val_len, uint32_t flags,
                  uint32_t expire, SetCallback callback) {
        CommandPtr command(new SetCommand(caller_loop, key, value, val_len , flags, expire, callback));

        caller_loop->RunInLoop(std::bind(&MemcacheClientPool::LaunchCommand, this, command));
    }

    void Remove(EventLoopPtr caller_loop, const char* key, RemoveCallback callback) {
        CommandPtr command(new RemoveCommand(caller_loop, key, callback));

        caller_loop->RunInLoop(std::bind(&MemcacheClientPool::LaunchCommand, this, command));
    }

    void Get(EventLoopPtr caller_loop, const char* key, GetCallback callback) {
        CommandPtr command(new GetCommand(caller_loop, key, callback));

        caller_loop->RunInLoop(std::bind(&MemcacheClientPool::LaunchCommand, this, command));
    }

    void MultiGet(EventLoopPtr caller_loop, const std::vector<std::string>& keys, MultiGetCallback callback) {
        LOG_INFO << "MultiGet keys=" << keys.size();
        CommandPtr command(new MultiGetCommand(caller_loop, keys, callback));
        caller_loop->RunInLoop(std::bind(&MemcacheClientPool::LaunchCommand, this, command));
    }
private:
    // noncopyable
    MemcacheClientPool(const MemcacheClientPool&);
    const MemcacheClientPool& operator=(const MemcacheClientPool&);

private:
    static void OnClientConnection(const evpp::TCPConnPtr& conn, MemcacheClient* memc_client) {
        LOG_INFO << "OnClientConnection conn=" << conn.get() << " memc_conn=" << memc_client->conn().get();
        if (conn->IsConnected()) {
            LOG_INFO << "OnClientConnection connect ok";
            CommandPtr command;
            while(command = memc_client->pop_waiting_command()) {
                memc_client->PushRunningCommand(command);
                command->Launch(memc_client);
                // command->caller_loop()->AssertInLoopThread();
            }
        } else {
            // TODO : 处理连接出错的情况
            LOG_INFO << "Disconnected from " << conn->remote_addr();
        }
    }

    void LaunchCommand(CommandPtr command) {
        LOG_INFO << "LaunchCommand";
        loop_pool_.GetNextLoopWithHash(command->thread_hash())->RunInLoop(std::bind(&MemcacheClientPool::DoLaunchCommand, this, command));
    }

private:
    void DoLaunchCommand(CommandPtr command) {
        std::map<std::string, MemcacheClient *>::iterator it = memc_clients_.find(command->ServerAddr());

        if (it == memc_clients_.end()) {
            evpp::TCPClient * tcp_client = new evpp::TCPClient(loop_pool_.GetNextLoopWithHash(command->thread_hash()), command->ServerAddr(), "MemcacheBinaryClient");
            MemcacheClient * memc_client = new MemcacheClient(loop_pool_.GetNextLoopWithHash(command->thread_hash()), tcp_client);
            LOG_INFO << "DoLaunchCommand new tcp_client=" << tcp_client << " memc_client=" << memc_client;

            // TODO : mem leak fix
            BinaryCodec* codec = new BinaryCodec(memc_client);

            tcp_client->SetConnectionCallback(std::bind(&MemcacheClientPool::OnClientConnection, std::placeholders::_1, memc_client));
            tcp_client->SetMesageHandler(std::bind(&BinaryCodec::OnCodecMessage, codec, std::placeholders::_1,
                                         std::placeholders::_2, std::placeholders::_3));
            tcp_client->Connect();

            memc_clients_[command->ServerAddr()] = memc_client;

            memc_client->push_waiting_command(command);
            return;
        }
        if (!it->second->conn() || !it->second->conn()->IsConnected()) {
            it->second->push_waiting_command(command);
            return;
        }
        it->second->PushRunningCommand(command);
        command->Launch(it->second);
    }

    thread_local static std::map<std::string, MemcacheClient *> memc_clients_;
    evpp::EventLoop loop_;
    evpp::EventLoopThreadPool loop_pool_;
};

thread_local std::map<std::string, MemcacheClient *> MemcacheClientPool::memc_clients_;

static void OnTestSetDone(const std::string key, int code) {
    LOG_INFO << "+++++++++++++ OnTestSetDone " << code << " " << key;
}
static void OnTestGetDone(const std::string key, GetResult res) {
    LOG_INFO << "============= OnTestGetDone " << key << " " << res.code << " " << res.value;
}
static void OnTestRemoveDone(const std::string key, int code) {
    LOG_INFO << "------------- OnTestRemoveDone " << code << " " << key;
}
static void OnTestMultiGetDone(MultiGetResult res) {
    LOG_INFO << ">>>>>>>>>>>>> OnTestMultiGetDone code=" << res.code;
    std::map<std::string, GetResult>::const_iterator it = res.get_result_map_.begin();
    for(; it != res.get_result_map_.end(); ++it) {
        LOG_INFO << ">>>>>>>>>>>>> OnTestMultiGetDone " << it->first << " " << it->second.code << " " << it->second.value;
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

// int main() {
TEST_UNIT(testMemcacheClient) {
    srand(time(NULL));
    std::thread th(evloop::MyEventThread);
    MemcacheClientPool mcp(8);

    const static int MAX_KEY = 1000;

    evpp::Duration g_timeout(0.30); // 300ms

#if 1
    for(size_t i = 0; i < MAX_KEY; i += 2) {
        std::stringstream ss_key;
        ss_key << "test" << i;
        std::stringstream ss_value;
        ss_value << "test_value" << i;
        mcp.Set(evloop::g_loop, ss_key.str(), ss_value.str(), &OnTestSetDone);
    }

    for(size_t i = 0; i < MAX_KEY; ++i) {
        std::stringstream ss;
        ss << "test" << i;
        mcp.Get(evloop::g_loop, ss.str().c_str(), &OnTestGetDone);
    }

    std::vector<std::string> mget_keys;
    for(size_t i = 0; i < MAX_KEY; ++i) {
        std::stringstream ss;
        ss << "test" << i;
        mget_keys.push_back(ss.str());
        if (mget_keys.size() >= 100) {
            break;
        }
    }
    mcp.MultiGet(evloop::g_loop, mget_keys, &OnTestMultiGetDone);

    for(size_t i = 0; i < MAX_KEY/2; ++i) {
        std::stringstream ss_key;
        ss_key << "test" << i;
        mcp.Remove(evloop::g_loop, ss_key.str().c_str(), &OnTestRemoveDone);
    }
#endif

    evloop::g_loop->RunAfter(1000.0, &evloop::StopLoop);
    th.join();
    // return 0;
}


