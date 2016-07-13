#pragma once

#include "evpp/inner_pre.h"
#include "evpp/event_loop.h"
#include "evpp/event_loop_thread_pool.h"
#include "evpp/tcp_callbacks.h"
#include "evpp/any.h"

#include <map>
#include <atomic>
#include <mutex>

namespace evpp {
class Connector;
class EVPP_EXPORT TCPClient {
public:
    TCPClient(EventLoop* loop, const std::string& remote_addr/*host:port*/, const std::string& name);
    ~TCPClient();
    void Connect();
    void Disconnect();
public:
    // ����һ��������صĻص����������ɹ��������ӡ������ӶϿ�����������ʧ�ܵ��¼�����ʱ��������øûص�
    //  ���ɹ���������ʱ���ص��еĲ��� TCPConn::IsConnected() == true
    //  �����ӶϿ�ʱ���ص��еĲ��� TCPConn::IsDisconnecting() == true
    //  ����������ʧ��ʱ���ص��еĲ��� TCPConn::IsDisconnected() == true ���� TCPConn::fd() == -1
    void SetConnectionCallback(const ConnectionCallback& cb) {
        conn_fn_ = cb;
    }

    void SetMessageCallback(const MessageCallback& cb) {
        msg_fn_ = cb;
    }
    void SetWriteCompleteCallback(const WriteCompleteCallback& cb) {
        write_complete_fn_ = cb;
    }
    void set_auto_reconnect(bool v) {
        auto_reconnect_.store(v);
    }
    void set_connecting_timeout(Duration timeout) {
        connecting_timeout_ = timeout;
    }
    void set_context(const Any& c) {
        context_ = c;
    }
    const Any& context() const {
        return context_;
    }
    TCPConnPtr conn() const;
    const std::string& remote_addr() const {
        return remote_addr_;
    }
    const std::string& name() const {
        return name_;
    }
private:
    void ConnectInLoop();
    void DisconnectInLoop();
    void OnConnection(int sockfd, const std::string& laddr);
    void OnRemoveConnection(const TCPConnPtr& conn);
    void Reconnect();
private:
    EventLoop* loop_;
    std::string remote_addr_;
    std::string name_;
    std::atomic<bool> auto_reconnect_; // �Զ�������ǣ�Ĭ��Ϊ true
    Any context_;

    mutable std::mutex mutex_; // The guard of conn_
    TCPConnPtr conn_;

    std::shared_ptr<Connector> connector_;
    Duration connecting_timeout_; // Default : 3 seconds

    ConnectionCallback conn_fn_;
    MessageCallback msg_fn_;
    WriteCompleteCallback write_complete_fn_;
};
}