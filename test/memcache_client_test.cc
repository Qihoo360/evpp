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

namespace {
/*
class MemecheClientPool {
  public:
    Connect()
    Set() 
    Get() 
    Close()

    Shutdown()

    void AsyncConnect(const std::string& addr, ConnectCB& callback);
    void AsyncSet(const char* key, const char* value, int val_len, int flags, evpp::Duration& expire, ConnectCB& callback) 
    void AsyncGet(const char* key, ConnectCB& callback)
  private:
    // clients_idle_;
    // clients_in_use_;
    evpp::EventLoopThreadPool loop_pool_;
};
*/

const static std::string addr = "127.0.0.1:11611";
static void OnMessage(const evpp::TCPConnPtr& conn,
               evpp::Buffer* msg,
               evpp::Timestamp ts) {
    LOG_INFO << msg->NextAllString();
}

void OnClientConnection(const evpp::TCPConnPtr& conn) {
    if (conn->IsConnected()) {
        conn->Send("set test1 0 3600 5 \r\n12345\r\n");
    } else {
        LOG_INFO << "Disconnected from " << conn->remote_addr();
    }
}

static void StartConnect(evpp::TCPClient * client) {
    client->SetConnectionCallback(&OnClientConnection);
    client->SetMesageHandler(&OnMessage);
    client->Connect();
}
static void ClearConn(evpp::TCPClient * client) {
    delete client;
}

}

TEST_UNIT(testMemcacheClient) {
    evpp::EventLoop loop;

    int thread_num = 24;
    evpp::EventLoopThreadPool pool(&loop, thread_num);
    H_TEST_ASSERT(pool.Start(true));

    evpp::TCPClient * client = new evpp::TCPClient(pool.GetNextLoopWithHash(100), addr, "MemcacheBinaryClient");
    pool.GetNextLoopWithHash(100)->RunInLoop(xstd::bind(&StartConnect, client));
    // client.Connect();

    usleep(1000*1000);
    pool.Stop(true);
    pool.GetNextLoopWithHash(100)->RunInLoop(xstd::bind(&ClearConn, client));
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




