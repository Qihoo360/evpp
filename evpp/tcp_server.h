#pragma once

#include "evpp/inner_pre.h"
#include "evpp/event_loop.h"
#include "evpp/event_loop_thread_pool.h"
#include "evpp/tcp_callbacks.h"

#include <map>

namespace evpp {
class Listener;

class EVPP_EXPORT TCPServer {
public:
    enum ThreadDispatchPolicy {
        kRoundRobin,
        kIPAddressHashing,
    };
    TCPServer(EventLoop* loop,
              const std::string& listen_addr/*ip:port*/,
              const std::string& name,
              int thread_num);
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

    void SetThreadDispatchPolicy(ThreadDispatchPolicy v) {
        threads_dispatch_policy_ = v;
    }
private:
    void StopInLoop();
    void RemoveConnection(const TCPConnPtr& conn);
    void RemoveConnectionInLoop(const TCPConnPtr& conn); // Remove the connection in the listener EventLoop
    void HandleNewConn(int sockfd, const std::string& remote_addr/*ip:port*/, const struct sockaddr_in* raddr);
    EventLoop* GetNextLoop(const struct sockaddr_in* raddr);
private:
    typedef std::map<std::string, TCPConnPtr> ConnectionMap;

    EventLoop* loop_;  // the listener loop
    const std::string listen_addr_;
    const std::string name_;
    std::shared_ptr<Listener> listener_;
    std::shared_ptr<EventLoopThreadPool> tpool_;
    MessageCallback msg_fn_;
    ConnectionCallback conn_fn_;
    WriteCompleteCallback write_complete_fn_;

    ThreadDispatchPolicy threads_dispatch_policy_;

    // always in loop thread
    uint64_t next_conn_id_;
    ConnectionMap connections_; // TODO �����������б�Ҫ����ô������ÿһ�����Ӷ�����libevent�ڲ����ü�¼�������ٱ���һ����������û��Ҫ�ġ������뿴����������һЩ��
};
}