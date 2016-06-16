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
// 1. pipeline
// 2. error handling
// 3. set/remove/mget
// 4. server addr hash
// 5. embeded & standalone

typedef std::shared_ptr<evpp::Buffer> BufferPtr;
typedef std::shared_ptr<evpp::EventLoop> EventLoopPtr;

typedef std::function<void(int code, const std::string key, const std::string value)> GetCallback;
typedef std::function<void(int code, const std::string key)> SetCallback;

typedef std::function<void(uint32_t command_id, int memcached_response_code)> RemoveCallback;
typedef std::function<void(uint32_t command_id, int memcached_response_code,
                      const std::string& return_value)> MultiGetOneResponseCallback;
typedef std::function<void(uint32_t noop_cmd_id, int memcached_response_code)> MultiGetCallback;

class MemcacheClient;
class Command  : public std::enable_shared_from_this<Command> {
  public:
    Command(EventLoopPtr ev_loop, const char* key) : ev_loop_(ev_loop), key_(key) {
    }
    virtual BufferPtr RequestBuffer() const = 0;

    std::string key_;
    EventLoopPtr ev_loop_;
    GetCallback get_callback_;
    SetCallback set_callback_;

    std::string ServerAddr() const {
        // const static std::string test_addr = "127.0.0.1:11611";
        const static std::string test_addr = "10.160.112.97:10002";
        return test_addr;
    }

    void Launch(MemcacheClient * memc_client);
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
        req.request.opcode = PROTOCOL_BINARY_CMD_GET;
        req.request.keylen = htons(key_.size());
        req.request.bodylen = htonl(key_.size());

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
    evpp::TCPConnPtr conn() { return tcp_client_->conn();}

    void AsyncGet(const char* key, GetCallback callback) {
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
    void OnMultiGetCommandDone(uint32_t noop_cmd_id, int memcached_response_code);
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
        command->ev_loop_->RunInLoop(std::bind(command->set_callback_, memcached_response_code,
                                               command->key_));
    } else {
        command->set_callback_(memcached_response_code, command->key_);
    }
}

void MemcacheClient::OnGetCommandDone(uint32_t command_id, int memcached_response_code, 
            const std::string& return_value) {
    CommandPtr command(command_running_.front());
    command_running_.pop();
    LOG_INFO << "OnGetCommandDone id=" << command_id << " code=" << memcached_response_code;
    if (command->ev_loop_) {
        command->ev_loop_->RunInLoop(std::bind(command->get_callback_, memcached_response_code,
                         command->key_, return_value));
    } else {
        command->get_callback_(memcached_response_code, command->key_, return_value);
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
            // memc_client_->onRemoveCommandDone(id, resp.response.status);
            break;
        case PROTOCOL_BINARY_CMD_GET:
            {
                const char* pv = buf->data() + sizeof(resp) + resp.response.extlen;
                std::string value(pv, resp.response.bodylen - resp.response.extlen);
                memc_client_->OnGetCommandDone(id, resp.response.status, value);
            }
            break;
        case PROTOCOL_BINARY_CMD_GETQ:
          //{
          //    const char* pv = buf->peek() + sizeof(resp) + resp.response.extlen;
          //    std::string value(pv, resp.response.bodylen - resp.response.extlen);
          //    memc_client_->onMultiGetCommandOneResponse(id, resp.response.status, value);
          //    //LOG_DEBUG << "GETQ, opaque=" << id << " value=" << value;
          //}
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
        // SetCommand(EventLoopPtr ev_loop, const char* key, const char * value, size_t val_len, uint32_t flags, uint32_t expire)

        CommandPtr command(new SetCommand(ev_loop, key, value, val_len , flags, expire));
        command->set_callback_ = callback;

        ev_loop->RunInLoop(std::bind(&MemcacheClientPool::LaunchCommand, this, command));
    }
    void AsyncGet(EventLoopPtr ev_loop, const char* key, GetCallback callback) {
        CommandPtr command(new GetCommand(ev_loop, key));
        command->get_callback_ = callback;

        ev_loop->RunInLoop(std::bind(&MemcacheClientPool::LaunchCommand, this, command));
    }
  private:
    // noncopyable
    MemcacheClientPool(const MemcacheClientPool&);
    const MemcacheClientPool& operator=(const MemcacheClientPool&);

    static void OnClientConnection(const evpp::TCPConnPtr& conn, MemcacheClient* memc_client) {
        if (conn->IsConnected()) {
            LOG_INFO << "OnClientConnection connect ok";
            if (!memc_client->command_waiting_.empty()) {
                CommandPtr command(memc_client->command_waiting_.front());
                memc_client->command_waiting_.pop();
                memc_client->command_running_.push(command);
                command->Launch(memc_client);

                LOG_INFO << "OnClientConnection begin send data";
                // conn->Send("set test1 0 3600 5 \r\n12345\r\n");
            }
        } else {
            LOG_INFO << "Disconnected from " << conn->remote_addr();
        }
    }

    void LaunchCommand(CommandPtr command) {
        loop_pool_.GetNextLoopWithHash(100)->RunInLoop(std::bind(&MemcacheClientPool::DoLaunchCommand, this, command));
    }

  private:
    void DoLaunchCommand(CommandPtr command) {
        if (memc_clients.find(command->ServerAddr()) == memc_clients.end()) {
            evpp::TCPClient * tcp_client = new evpp::TCPClient(loop_pool_.GetNextLoopWithHash(100), command->ServerAddr(), "MemcacheBinaryClient");
            MemcacheClient * memc_client = new MemcacheClient(tcp_client);
            BinaryCodec* codec = new BinaryCodec(memc_client);

            tcp_client->SetConnectionCallback(std::bind(&MemcacheClientPool::OnClientConnection, std::placeholders::_1, memc_client));
            tcp_client->SetMesageHandler(std::bind(&BinaryCodec::OnCodecMessage, codec, std::placeholders::_1,
                                         std::placeholders::_2, std::placeholders::_3));
            tcp_client->Connect();

            memc_clients[command->ServerAddr()] = memc_client;

            memc_client->command_waiting_.push(command);
            return;
        }

        command->Launch(memc_clients[command->ServerAddr()]);
    }

    thread_local static std::map<std::string, MemcacheClient *> memc_clients;
    evpp::EventLoop loop_;
    evpp::EventLoopThreadPool loop_pool_;
};

thread_local std::map<std::string, MemcacheClient *> MemcacheClientPool::memc_clients;

static void OnTestSetDone(int code, const std::string key) {
    LOG_INFO << "OnTestSetDone " << code << " " << key;
}
static void OnTestGetDone(int code, const std::string key, const std::string value) {
    LOG_INFO << "OnTestGetDone " << code << " " << key << " " << value;
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

TEST_UNIT(testMemcacheClient) {
    std::thread th(evloop::MyEventThread);

    MemcacheClientPool mcp(16);
    // mcp.AsyncSet(evloop::g_loop, "test1", "test1_value", &OnTestSetDone);
    for(size_t i = 0; i < 100; ++i) {
        mcp.AsyncGet(evloop::g_loop, "test1", &OnTestGetDone);
    }

    evloop::g_loop->RunAfter(1000.0, &evloop::StopLoop);
    th.join();
    // usleep(100*1000);
}

