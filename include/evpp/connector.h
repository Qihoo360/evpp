#include "evpp/inner_pre.h"
#include "duration.h"

namespace evpp {
    class EventLoop;
    class FdChannel;
    class TimerEventWatcher;
    class EVPP_EXPORT Connector {
    public:
        typedef xstd::function<void(int sockfd, const std::string&/*local addr*/)> NewConnectionCallback;
        Connector(EventLoop* loop, const std::string& remote_addr, Duration timeout);
        ~Connector();
        void Start();
    public:
        void SetNewConnectionCallback(NewConnectionCallback cb) { conn_fn_ = cb; }
        bool IsConnecting() const { return status_ == kConnecting; }
    private:
        void HandleWrite();
        void HandleError();
        void OnConnectTimeout();
    private:
        enum Status { kDisconnected, kConnecting, kConnected };
        Status status_;
        EventLoop* loop_;
        std::string remote_addr_;
        struct sockaddr_in raddr_;
        Duration timeout_;
        xstd::shared_ptr<FdChannel> chan_;
        xstd::shared_ptr<TimerEventWatcher> timer_;
        NewConnectionCallback conn_fn_;
    };
}