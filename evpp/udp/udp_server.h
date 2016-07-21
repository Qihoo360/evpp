#pragma once

#include "evpp/inner_pre.h"
#include "udp_message.h"

namespace evpp {
namespace udp {

class EVPP_EXPORT Server {
public:
    typedef std::function<void(const MessagePtr& msg)> MessageHandler;
    enum Status {
        kRunning = 1,
        kStopping = 2,
        kStopped = 3,
    };
public:
    Server();
    ~Server();

    //! Start the server.
    //! \remark Start the service and listening in the given port
    //!		This call will start several receiving thread at every net interface
    //! \return false if failed to start server.
    bool Start(std::vector<int> ports);
    bool Start(int port);

    //! Stop the server
    void Stop();
    void Stop(bool wait_thread_exit);

    bool IsRunning() const;
    bool IsStopped() const;

    void SetMessageHandler(MessageHandler handler);

public:
    void set_name(const std::string& n);

private:
    class Impl;
    std::shared_ptr<Impl> impl_;
};

}
}
