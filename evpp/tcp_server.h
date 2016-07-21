#pragma once

#include "evpp/inner_pre.h"
#include "evpp/event_loop.h"
#include "evpp/event_loop_thread_pool.h"
#include "evpp/tcp_callbacks.h"
#include "evpp/thread_dispatch_policy.h"

#include <map>

namespace evpp {
class Listener;

class EVPP_EXPORT TCPServer : public ThreadDispatchPolicy {
public:
    TCPServer(EventLoop* loop,
              const std::string& listen_addr/*ip:port*/,
              const std::string& name,
              uint32_t thread_num);
    ~TCPServer();
    bool Start();
    void Stop();

    // ����һ��������صĻص������������յ�һ���µ����ӡ����������ӶϿ����¼�����ʱ��������øûص�
    //  ���ɹ���������ʱ���ص��еĲ��� TCPConn::IsConnected() == true
    //  �����ӶϿ�ʱ���ص��еĲ��� TCPConn::IsDisconnecting() == true
    void SetConnectionCallback(const ConnectionCallback& cb) {
        conn_fn_ = cb;
    }

    void SetMessageCallback(MessageCallback cb) {
        msg_fn_ = cb;
    }

private:
    void StopInLoop();
    void RemoveConnection(const TCPConnPtr& conn);
    void HandleNewConn(int sockfd, const std::string& remote_addr/*ip:port*/, const struct sockaddr_in* raddr);
    EventLoop* GetNextLoop(const struct sockaddr_in* raddr);
private:
    typedef std::map<std::string, TCPConnPtr> ConnectionMap;

    EventLoop* loop_;  // the listener loop
    const std::string listen_addr_;
    const std::string name_;
    std::unique_ptr<Listener> listener_;
    std::shared_ptr<EventLoopThreadPool> tpool_;
    MessageCallback msg_fn_;
    ConnectionCallback conn_fn_;
    WriteCompleteCallback write_complete_fn_;

    // always in loop thread
    uint64_t next_conn_id_;
    ConnectionMap connections_;
};
}