#include "evnsq.h"

#include <evpp/event_loop.h>
#include <evpp/tcp_client.h>
#include <evpp/tcp_conn.h>

namespace evnsq {
    NSQClient::NSQClient(evpp::EventLoop* loop, const std::string& raddr)
        : loop_(loop) {
        tcpc_.reset(new evpp::TCPClient(loop_, raddr, "NSQClient"));
        tcpc_->SetConnectionCallback(std::bind(&NSQClient::OnConnection, this, std::placeholders::_1));
    }

    NSQClient::~NSQClient() {
    }

    bool NSQClient::Connect() {
        tcpc_->Connect();
        return false;
    }

    void NSQClient::OnConnection(const evpp::TCPConnPtr& conn) {
        if (conn->IsConnected()) {
            static const std::string nsq_magic = "  V2";
            conn->Send(nsq_magic);
        } else {
            //TODO
            LOG_ERROR << "Connect to " << tcpc_->remote_addr() << " failed.";
        }
    }

}