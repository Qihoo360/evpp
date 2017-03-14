// Modified from https://github.com/chenshuo/muduo/blob/master/examples/pingpong/client.cc

#include <evpp/exp.h>
#include <evpp/tcp_client.h>
#include <evpp/buffer.h>
#include <evpp/tcp_conn.h>

class Client;

class Session {
public:
    Session(evpp::EventLoop* loop,
            const std::string& serverAddr/*ip:port*/,
            const std::string& name,
            Client* owner)
        : client_(loop, serverAddr, name),
        owner_(owner),
        bytes_read_(0),
        bytes_written_(0),
        messages_read_(0) {
        client_.SetConnectionCallback(
            std::bind(&Session::OnConnection, this, std::placeholders::_1));
        client_.SetMessageCallback(
            std::bind(&Session::OnMessage, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
    }

    void Start() {
        client_.Connect();
    }

    void Stop() {
        client_.Disconnect();
    }

    int64_t bytes_read() const {
        return bytes_read_;
    }

    int64_t messages_read() const {
        return messages_read_;
    }

private:
    void OnConnection(const evpp::TCPConnPtr& conn);

    void OnMessage(const evpp::TCPConnPtr& conn, evpp::Buffer* buf, evpp::Timestamp) {
        ++messages_read_;
        bytes_read_ += buf->size();
        bytes_written_ += buf->size();
        conn->Send(buf);
    }

private:
    evpp::TCPClient client_;
    Client* owner_;
    int64_t bytes_read_;
    int64_t bytes_written_;
    int64_t messages_read_;
};

class Client {
public:
    Client(evpp::EventLoop* loop,
           const std::string& name,
           const std::string& serverAddr, // ip:port
           int blockSize,
           int sessionCount,
           int timeout_sec,
           int threadCount)
        : loop_(loop),
        name_(name),
        session_count_(sessionCount),
        timeout_(timeout_sec),
        connected_count_(0) {
        loop->RunAfter(evpp::Duration(double(timeout_sec)), std::bind(&Client::HandleTimeout, this));
        tpool_.reset(new evpp::EventLoopThreadPool(loop, threadCount));
        tpool_->Start(true);

        for (int i = 0; i < blockSize; ++i) {
            message_.push_back(static_cast<char>(i % 128));
        }

        for (int i = 0; i < sessionCount; ++i) {
            char buf[32];
            snprintf(buf, sizeof buf, "C%05d", i);
            Session* session = new Session(tpool_->GetNextLoop(), serverAddr, buf, this);
            session->Start();
            sessions_.push_back(session);
        }
    }

    ~Client() {
    }

    const std::string& message() const {
        return message_;
    }

    void OnConnect() {
        if (++connected_count_ == session_count_) {
            LOG_WARN << "all connected";
        }
    }

    void OnDisconnect(const evpp::TCPConnPtr& conn) {
        if (--connected_count_ == 0) {
            LOG_WARN << "all disconnected";

            int64_t totalBytesRead = 0;
            int64_t totalMessagesRead = 0;
            for (auto &it : sessions_) {
                totalBytesRead += it->bytes_read();
                totalMessagesRead += it->messages_read();
            }
            LOG_WARN << "name=" << name_ << " " << totalBytesRead << " total bytes read";
            LOG_WARN << "name=" << name_ << " " << totalMessagesRead << " total messages read";
            LOG_WARN << "name=" << name_ << " " << static_cast<double>(totalBytesRead) / static_cast<double>(totalMessagesRead) << " average message size";
            LOG_WARN << "name=" << name_ << " " << static_cast<double>(totalBytesRead) / (timeout_ * 1024 * 1024) << " MiB/s throughput";
            loop_->QueueInLoop(std::bind(&Client::Quit, this));
        }
    }

private:
    void Quit() {
        tpool_->Stop();
        loop_->Stop();
        for (auto &it : sessions_) {
            delete it;
        }
        sessions_.clear();
        while (!tpool_->IsStopped() || !loop_->IsStopped()) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
        tpool_.reset();
    }

    void HandleTimeout() {
        LOG_WARN << "stop";
        for (auto &it : sessions_) {
            it->Stop();
        }
    }
private:
    evpp::EventLoop* loop_;
    std::string name_;
    std::shared_ptr<evpp::EventLoopThreadPool> tpool_;
    int session_count_;
    int timeout_;
    std::vector<Session*> sessions_;
    std::string message_;
    std::atomic<int> connected_count_;
};

void Session::OnConnection(const evpp::TCPConnPtr& conn) {
    if (conn->IsConnected()) {
        conn->SetTCPNoDelay(true);
        conn->Send(owner_->message());
        owner_->OnConnect();
    } else {
        owner_->OnDisconnect(conn);
    }
}

int main(int argc, char* argv[]) {
    google::InitGoogleLogging(argv[0]);
    FLAGS_stderrthreshold = 0;
    if (argc != 7) {
        fprintf(stderr, "Usage: client <host_ip> <port> <threads> <blocksize> <sessions> <time_seconds>\n");
        return -1;
    }

    const char* ip = argv[1];
    uint16_t port = static_cast<uint16_t>(atoi(argv[2]));
    int threadCount = atoi(argv[3]);
    int blockSize = atoi(argv[4]);
    int sessionCount = atoi(argv[5]);
    int timeout = atoi(argv[6]);

    evpp::EventLoop loop;
    std::string serverAddr = std::string(ip) + ":" + std::to_string(port);

    Client client(&loop, argv[0], serverAddr, blockSize, sessionCount, timeout, threadCount);
    loop.Run();
    return 0;
}




#include "../../../examples/echo/tcpecho/winmain-inl.h"





