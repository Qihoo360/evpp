
#include "evpp/exp.h"
#include "test_common.h"

#include "evpp/libevent_headers.h"
#include "evpp/libevent_watcher.h"
#include "evpp/event_loop.h"
#include "evpp/tcp_server.h"
#include "evpp/buffer.h"
#include "evpp/tcp_conn.h"

#include <thread>

// namespace {
//     static void OnMessage(const evpp::TCPConnPtr& conn,
//                    evpp::Buffer* msg,
//                    evpp::Timestamp ts) {
//         std::string s = msg->NextAllString();
//         LOG_INFO << "Recv a message [" << s << "]";
//         conn->Send(s.data(), s.size());
//     }
// }
// 
// 
// TEST_UNIT(testATCPServer) {
//     evpp::EventLoop loop;
//     evpp::TCPServer tsrv(&loop, "127.0.0.1:9099", "tcp_server", 1);
//     tsrv.SetMesageHandler(&OnMessage);
//     tsrv.Start();
//     loop.Run();
// }


