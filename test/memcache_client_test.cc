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

typedef xstd::function<void(char*)> CommandCallback;
typedef xstd::function<void(evpp::Slice)> GetCallback;
typedef xstd::function<void(char*)> MgetCallback;
typedef xstd::function<void(char*)> StoreCallback;
typedef xstd::function<void(char*)> RemoveCallback;

class Command  : public xstd::enable_shared_from_this<Command> {
  public:
    Command() {
    }
    PacketHeader header_;
    std::string body_;

    GetCallback get_callback_;

    static void OnCommandMessage(const evpp::TCPConnPtr& conn,
               evpp::Buffer* msg,
               evpp::Timestamp ts);

    void Run(evpp::TCPClient * client) {
        client->conn()->Send((const void*)&header_, sizeof(header_));
        client->conn()->Send((const void*)body_.c_str(), body_.size());
    }
};

typedef xstd::shared_ptr<Command> CommandPtr;
thread_local std::queue<CommandPtr> g_command_waiting;
thread_local std::queue<CommandPtr> g_command_running;
thread_local evpp::TCPClient * tcp_client_;


void Command::OnCommandMessage(const evpp::TCPConnPtr& conn,
           evpp::Buffer* msg,
           evpp::Timestamp ts) {
    if (msg->size() >= sizeof(PacketHeader)) { // TODO: 完整接收
        CommandPtr command(g_command_running.front());
        g_command_running.pop();
        PacketHeader rsp_header(msg->Next(sizeof(PacketHeader)));
        LOG_INFO << "response body length = " << ntohl(rsp_header.total_body_length);
        command->get_callback_(msg->Next(ntohl(rsp_header.total_body_length)));
    }
}

const static std::string addr = "127.0.0.1:11611";

void OnClientConnection(const evpp::TCPConnPtr& conn, evpp::TCPClient* client) {
    if (conn->IsConnected()) {
        LOG_INFO << "OnClientConnection connect ok";
        if (!g_command_waiting.empty()) {
            CommandPtr command(g_command_waiting.front());
            g_command_waiting.pop();
            g_command_running.push(command);
            command->Run(client);

            LOG_INFO << "OnClientConnection begin send data";
            // conn->Send("set test1 0 3600 5 \r\n12345\r\n");
        }
    } else {
        LOG_INFO << "Disconnected from " << conn->remote_addr();
    }
}

class MemcacheClient {
  public:
    MemcacheClient(int thread_count) : loop_pool_(&loop_, thread_count) {
        loop_pool_.Start(true);
    }
    virtual ~MemcacheClient() {
        loop_pool_.Stop(true);
    }
    // void AsyncSet(const char* key, const char* value, int val_len, int flags,
    //               evpp::Duration& expire, ConnectCB& callback)
    void AsyncGet(const char* key, GetCallback callback) {
        CommandPtr command(new Command);
        command->header_.magic = 0x80;
        command->header_.op_code = OP_GET;

        command->header_.key_length = htons(strlen(key));
        command->header_.total_body_length = htonl(strlen(key));
        command->body_ = key;
        command->get_callback_ = callback;
        
        loop_pool_.GetNextLoopWithHash(100)->RunInLoop(xstd::bind(&MemcacheClient::ExecuteCommand, this, command));
    }
  private:
    void ExecuteCommand(CommandPtr command) {
        if (tcp_client_ == NULL ) {
            // TODO : 限制最大连接数
            tcp_client_= new evpp::TCPClient(loop_pool_.GetNextLoopWithHash(100), addr, "MemcacheBinaryClient");
            tcp_client_->SetConnectionCallback(xstd::bind(&OnClientConnection, xstd::placeholders::_1, tcp_client_));
            tcp_client_->SetMesageHandler(&Command::OnCommandMessage);
            tcp_client_->Connect();

            g_command_waiting.push(command);
            return;
        }

        command->Run(tcp_client_);
    }

    evpp::EventLoop loop_;
    evpp::EventLoopThreadPool loop_pool_;

};


static void OnTestGetDone(evpp::Slice slice) {
    LOG_INFO << slice.data();
}

}

TEST_UNIT(testMemcacheClient) {
    MemcacheClient memc_client(16);
    const char * key = "test1";
    memc_client.AsyncGet(key, &OnTestGetDone);

    usleep(100*1000);
}

///////////////////////////////////////////////////////

static void StartConnect(evpp::TCPClient * client) {
    client->SetConnectionCallback(xstd::bind(&OnClientConnection, xstd::placeholders::_1, client));
    client->Connect();
}

static void ClearConn(evpp::TCPClient * client) {
    delete client;
}

TEST_UNIT(testMemcacheClient2) {
    evpp::EventLoopThread loop_thread;

    evpp::TCPClient * client = new evpp::TCPClient(loop_thread.event_loop(), addr, "MemcacheBinaryClient");
    loop_thread.event_loop()->RunInLoop(xstd::bind(&StartConnect, client));
    // client.Connect();

    usleep(1000*1000);
    loop_thread.Stop(true);
    loop_thread.event_loop()->RunInLoop(xstd::bind(&ClearConn, client));
    usleep(1000 * 1000);
}


