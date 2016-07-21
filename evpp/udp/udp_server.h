#pragma once

#include "evpp/inner_pre.h"
#include "udp_message.h"

#include <thread>

namespace evpp {
namespace udp {
class EVPP_EXPORT Server {
public:
    typedef std::function<void(const MessagePtr& msg)> MessageHandler;
public:
    Server();
    ~Server();

    bool Start(int port);
    bool Start(std::vector<int> ports);
    void Stop(bool wait_thread_exit);

    void Pause();
    void Continue();

    bool IsRunning() const;
    bool IsStopped() const;

    void SetMessageHandler(MessageHandler handler) {
        message_handler_ = handler;
    }

private:
    class RecvThread;
    typedef std::shared_ptr<RecvThread> RecvThreadPtr;
    std::vector<RecvThreadPtr> recv_threads_;

    MessageHandler   message_handler_;
private:
    void RecvingLoop(RecvThread* th);
};

}
}
