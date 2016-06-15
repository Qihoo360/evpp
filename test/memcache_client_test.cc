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

namespace {
enum PacketOpCode {
    OP_GET = 0x00,
    OP_SET = 0x01,
    OP_ADD = 0x02,
    OP_REPLACE = 0x03,
    OP_DELETE = 0x04,
    OP_INCREMENT = 0x05,
    OP_DECREMENT = 0x06,
    // ...
};

#pragma pack(1)
struct PacketHeader {
    PacketHeader() {
        memset(&magic, sizeof(PacketHeader), 0);
    }
    PacketHeader(evpp::Slice slice) {
        memcpy(&magic, slice.data(), sizeof(PacketHeader));
    }
//  uint16_t key_length() const {
//      return key_length_;
//  }
//  void set_key_length(uint16_t v) {
//      key_length = htons(v);
//  }

    uint8_t magic;
    uint8_t op_code;
    uint16_t key_length;

    uint8_t extra_length;
    uint8_t data_type;
    uint16_t status;

    uint32_t total_body_length;
    uint32_t opaque;
    uint64_t cas;
};
#pragma pack()

typedef xstd::shared_ptr<evpp::EventLoop> EventLoopPtr;

typedef xstd::function<void(char*)> CommandCallback;
typedef xstd::function<void(evpp::Slice)> GetCallback;
typedef xstd::function<void(char*)> MgetCallback;
typedef xstd::function<void(char*)> StoreCallback;
typedef xstd::function<void(char*)> RemoveCallback;

class MemcacheClient;
class Command  : public xstd::enable_shared_from_this<Command> {
  public:
    Command(EventLoopPtr ev_loop) : ev_loop_(ev_loop) {
    }
    PacketHeader header_;
    std::string body_;

    GetCallback get_callback_;
    EventLoopPtr ev_loop_;
    std::string ServerAddr() const {
        const static std::string test_addr = "127.0.0.1:11611";
        return test_addr;
    }

    static void OnCommandMessage(const evpp::TCPConnPtr& conn,
               evpp::Buffer* msg,
               evpp::Timestamp ts,
               MemcacheClient * memc_client);
    void Run(MemcacheClient * client);
};

typedef xstd::shared_ptr<Command> CommandPtr;

class MemcacheClient {
  public:
    MemcacheClient(evpp::TCPClient * tcp_client) : tcp_client_(tcp_client) {}

    std::queue<CommandPtr> command_waiting_;
    std::queue<CommandPtr> command_running_;
    evpp::TCPClient * tcp_client_;
  private:
    // noncopyable
    MemcacheClient(const MemcacheClient&);
    const MemcacheClient& operator=(const MemcacheClient&);
};

void Command::Run(MemcacheClient * client) {
    client->tcp_client_->conn()->Send((const void*)&header_, sizeof(header_));
    client->tcp_client_->conn()->Send((const void*)body_.c_str(), body_.size());
}

void Command::OnCommandMessage(const evpp::TCPConnPtr& conn,
           evpp::Buffer* msg,
           evpp::Timestamp ts,
           MemcacheClient * memc_client) {
    if (msg->size() >= sizeof(PacketHeader)) { // TODO: 完整接收
        CommandPtr command(memc_client->command_running_.front());
        memc_client->command_running_.pop();
        PacketHeader rsp_header(msg->Next(sizeof(PacketHeader)));
        LOG_INFO << "response body length = " << ntohl(rsp_header.total_body_length);

        command->ev_loop_->RunInLoop(xstd::bind(command->get_callback_, msg->Next(ntohl(rsp_header.total_body_length))));

        // ev_loop->RunInLoop(xstd::bind(&MemcacheClientPool::DoAsyncGet, this, command));
        // command->get_callback_(msg->Next(ntohl(rsp_header.total_body_length)));
    }
}

void OnClientConnection(const evpp::TCPConnPtr& conn, MemcacheClient* client) {
    if (conn->IsConnected()) {
        LOG_INFO << "OnClientConnection connect ok";
        if (!client->command_waiting_.empty()) {
            CommandPtr command(client->command_waiting_.front());
            client->command_waiting_.pop();
            client->command_running_.push(command);
            command->Run(client);

            LOG_INFO << "OnClientConnection begin send data";
            // conn->Send("set test1 0 3600 5 \r\n12345\r\n");
        }
    } else {
        LOG_INFO << "Disconnected from " << conn->remote_addr();
    }
}


class MemcacheClientPool {
  public:
    MemcacheClientPool(int conncurrency) : loop_pool_(&loop_, conncurrency) {
        loop_pool_.Start(true);
    }
    virtual ~MemcacheClientPool() {
        loop_pool_.Stop(true);
    }
    // void AsyncSet(const char* key, const char* value, int val_len, int flags,
    //               evpp::Duration& expire, ConnectCB& callback)
    void AsyncGet(EventLoopPtr ev_loop, const char* key, GetCallback callback) {
        CommandPtr command(new Command(ev_loop));
        command->header_.magic = 0x80;
        command->header_.op_code = OP_GET;

        command->header_.key_length = htons(strlen(key));
        command->header_.total_body_length = htonl(strlen(key));
        command->body_ = key;
        command->get_callback_ = callback;

        ev_loop->RunInLoop(xstd::bind(&MemcacheClientPool::LaunchCommand, this, command));
    }
  private:
    // noncopyable
    MemcacheClientPool(const MemcacheClientPool&);
    const MemcacheClientPool& operator=(const MemcacheClientPool&);

    void LaunchCommand(CommandPtr command) {
        // 如何获取当前event loop ?
        // static xstd::shared_ptr<evpp::EventLoop> loop;
        
        loop_pool_.GetNextLoopWithHash(100)->RunInLoop(xstd::bind(&MemcacheClientPool::ExecuteCommand, this, command));
    }

  private:
    void ExecuteCommand(CommandPtr command) {
        if (memc_clients.find(command->ServerAddr()) == memc_clients.end()) {
            evpp::TCPClient * tcp_client = new evpp::TCPClient(loop_pool_.GetNextLoopWithHash(100), command->ServerAddr(), "MemcacheBinaryClient");
            MemcacheClient * memc_client = new MemcacheClient(tcp_client);

            tcp_client->SetConnectionCallback(xstd::bind(&OnClientConnection, xstd::placeholders::_1, memc_client));
            tcp_client->SetMesageHandler(xstd::bind(&Command::OnCommandMessage, xstd::placeholders::_1,
                                         xstd::placeholders::_2, xstd::placeholders::_3, memc_client));
            tcp_client->Connect();

            memc_clients[command->ServerAddr()] = memc_client;

            memc_client->command_waiting_.push(command);
            return;
        }

        command->Run(memc_clients[command->ServerAddr()]);
    }

    thread_local static std::map<std::string, MemcacheClient *> memc_clients;
    evpp::EventLoop loop_;
    evpp::EventLoopThreadPool loop_pool_;
};

thread_local std::map<std::string, MemcacheClient *> MemcacheClientPool::memc_clients;


// TODO : 回调在外部处理，thread pool 只处理网络IO时间 ?
static void OnTestGetDone(evpp::Slice slice) {
    LOG_INFO << "OnTestGetDone " << slice.data();
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
    mcp.AsyncGet(evloop::g_loop, "test1", &OnTestGetDone);

    evloop::g_loop->RunAfter(1000.0, &evloop::StopLoop);
    th.join();
    // usleep(100*1000);
}

