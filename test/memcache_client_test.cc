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
#include <list>

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
    PacketHeader(evpp::Slice & slice) {
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
typedef xstd::function<void(char*)> GetCallback;

class Command  : public xstd::enable_shared_from_this<Command> {
  public:
    Command() {
    }
    PacketHeader header_;
    std::string body_;
    CommandCallback callback_;

    static void OnCommandMessage(const evpp::TCPConnPtr& conn,
               evpp::Buffer* msg,
               evpp::Timestamp ts) {
        if (msg->size() >= sizeof(PacketHeader)) {
            LOG_INFO << msg->NextAllString();
            // 
        }
    }

    void Run(evpp::TCPClient * client) {
        // can't set message handler on the fly!
        // client->SetMesageHandler(xstd::bind(&Command::OnCommandMessage, shared_from_this(), xstd::placeholders::_1, xstd::placeholders::_2, xstd::placeholders::_3));

        client->conn()->Send((const void*)&header_, sizeof(header_));
        client->conn()->Send((const void*)body_.c_str(), body_.size());
    }
};

typedef xstd::shared_ptr<Command> CommandPtr;

thread_local std::list<CommandPtr> g_command_queue;
thread_local std::list<evpp::TCPClient *> g_idle_clients;
thread_local std::list<evpp::TCPClient *> g_busy_clients;

evpp::EventLoop loop;
int thread_num = 24;
evpp::EventLoopThreadPool pool(&loop, thread_num);

class MemecheClientPool {
  public:
    // void AsyncSet(const char* key, const char* value, int val_len, int flags, evpp::Duration& expire, ConnectCB& callback) 
    void AsyncGet(const char* key, GetCallback& callback);
  private:
    // command_queue_;

    // clients_idle_;
    // clients_in_use_;
    evpp::EventLoopThreadPool loop_pool_;
};

const static std::string addr = "127.0.0.1:11611";
void OnSetMessage(const evpp::TCPConnPtr& conn,
               evpp::Buffer* msg,
               evpp::Timestamp ts) {
    LOG_INFO << msg->NextAllString();
}

void OnClientConnection(const evpp::TCPConnPtr& conn, evpp::TCPClient* client) {
    if (conn->IsConnected()) {
        if (g_command_queue.empty()) {
            // 如何将client放到队列中
            g_idle_clients.push_back(client);
        } else {
            CommandPtr command(g_command_queue.front());
            g_command_queue.pop_front();
            command->Run(client);
            // conn->Send("set test1 0 3600 5 \r\n12345\r\n");
        }
    } else {
        LOG_INFO << "Disconnected from " << conn->remote_addr();
    }
}

static void Execute(CommandPtr command) {
    evpp::TCPClient * client = NULL;
    if (g_idle_clients.empty()) {
        // TODO : 限制最大连接数
        client = new evpp::TCPClient(pool.GetNextLoopWithHash(100), addr, "MemcacheBinaryClient");
        client->SetConnectionCallback(xstd::bind(&OnClientConnection, xstd::placeholders::_1, client));
        client->SetMesageHandler(&Command::OnCommandMessage);
        client->Connect();

        g_command_queue.push_back(command);
    } else {
        client = g_idle_clients.front();
        g_idle_clients.pop_front();
        command->Run(client);
    }
}

static void StartConnect(evpp::TCPClient * client) {
    client->SetConnectionCallback(xstd::bind(&OnClientConnection, xstd::placeholders::_1, client));
    client->Connect();
}

static void ClearConn(evpp::TCPClient * client) {
    delete client;
}

}

TEST_UNIT(testMemcacheClient) {
    H_TEST_ASSERT(pool.Start(true));

    // evpp::TCPClient * client = new evpp::TCPClient(pool.GetNextLoopWithHash(100), addr, "MemcacheBinaryClient");
    CommandPtr command(new Command);
    command->header_.magic = 0x80;
    command->header_.op_code = OP_GET;
    command->header_.key_length = htons(5);
    command->header_.total_body_length = htonl(5);
    command->body_ = "test1";
    // command->callback_ = xxx;

    pool.GetNextLoopWithHash(100)->RunInLoop(xstd::bind(&Execute, command));
    // client.Connect();

    usleep(1000*1000);
    pool.Stop(true);
    // pool.GetNextLoopWithHash(100)->RunInLoop(xstd::bind(&ClearConn, client));
    usleep(1000);
}

TEST_UNIT(testMemcacheClient2) {
    evpp::EventLoopThread loop_thread;
    loop_thread.Start(true);

    evpp::TCPClient * client = new evpp::TCPClient(loop_thread.event_loop(), addr, "MemcacheBinaryClient");
    loop_thread.event_loop()->RunInLoop(xstd::bind(&StartConnect, client));
    // client.Connect();

    usleep(1000*1000);
    loop_thread.Stop(true);
    loop_thread.event_loop()->RunInLoop(xstd::bind(&ClearConn, client));
    usleep(1000 * 1000);
}




