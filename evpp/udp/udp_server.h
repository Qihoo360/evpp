#pragma once

#include "evpp/inner_pre.h"
#include "udp_message.h"

#include <thread>

namespace evpp {
namespace udp {


class RecvThread;
class EVPP_EXPORT Server {
public:
    typedef std::function<void(const MessagePtr& msg)> MessageHandler;
public:
    Server();
    ~Server();

    bool Start(int port);
    bool Start(std::vector<int> ports);

    void Stop(bool wait_thread_exit);

    bool IsRunning() const;
    bool IsStopped() const;

    void SetMessageHandler(MessageHandler handler) {
        message_handler_ = handler;
    }

private:
    typedef std::shared_ptr<RecvThread> RecvThreadPtr;

    typedef std::vector<RecvThreadPtr> RecvThreadVector;
    RecvThreadVector recv_threads_;
    MessageHandler   message_handler_;

private:
    friend class RecvThread;
    void RecvingLoop(RecvThread* th);
};

}
}
