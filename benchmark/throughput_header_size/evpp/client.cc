// Modified from https://github.com/huyuguang/asio_benchmark/blob/master/client3.cpp

#include <evpp/exp.h>
#include <evpp/tcp_client.h>
#include <evpp/buffer.h>
#include <evpp/tcp_conn.h>
#include <evpp/timestamp.h>

#include "header.h"

class Client;

class Session : public std::enable_shared_from_this<Session> {
public:
    Session(evpp::EventLoop* loop,
            const std::string& serverAddr/*ip:port*/,
            const std::string& name,
            const size_t total_count,
            Client* owner)
        : client_(loop, serverAddr, name),
        owner_(owner),
        total_count_(total_count) {
        client_.SetConnectionCallback(
            std::bind(&Session::OnConnection, this, std::placeholders::_1));
        client_.SetMessageCallback(
            std::bind(&Session::OnMessage, this, std::placeholders::_1, std::placeholders::_2));
        client_.set_connecting_timeout(evpp::Duration(10.0));
        memset(buffer_, 'x', sizeof(buffer_));
    }

    void Start() {
        client_.Connect();
    }

    void Stop() {
        client_.Disconnect();
    }

    bool finished() const {
        return finished_;
    }

    evpp::Duration use_time() const {
        return stop_time_ - start_time_;
    }
private:
    uint32_t get_body_len() {
        if (body_len_ > kMaxSize)
            body_len_ = 100;
        return body_len_++;
    }

    bool check_count(Header* header) {
        return (ntohl(header->packet_count_) >= total_count_);
    }
private:
    void OnConnection(const evpp::TCPConnPtr& conn);

    void OnMessage(const evpp::TCPConnPtr& conn, evpp::Buffer* buf) {
        LOG_INFO << " buf->size=" << buf->size();
        const size_t kHeaderLen = sizeof(Header);
        while (buf->size() > kHeaderLen) {
            Header* header = reinterpret_cast<Header*>(const_cast<char*>(buf->data()));
            auto full_size = header->get_full_size();
            if (buf->size() < full_size) {
                // need to read more data
                return;
            }

            LOG_INFO << "full_size=" << full_size << " header.body_size_=" << ntohl(header->body_size_) << " header.packet_count_=" << ntohl(header->packet_count_);

            if (check_count(header)) {
                stop_time_ = evpp::Timestamp::Now();
                finished_ = true;
                LOG_INFO << "stopping session " << client_.name();
                client_.event_loop()->RunInLoop(std::bind(&Session::Stop, shared_from_this()));
                break;
            } else {
                header->body_size_ = htonl(get_body_len());
                header->inc_packet_count();
                conn->Send(buf->data(), full_size + 1); // trick here
                buf->Skip(full_size); // only skip full_size, not full_size+1
            }
        }
    }

private:
    evpp::TCPClient client_;
    evpp::Timestamp start_time_;
    evpp::Timestamp stop_time_;
    Client* owner_;
    bool finished_ = false;
    const uint32_t total_count_ = 100;
    uint32_t body_len_ = 100;
    static size_t const max_block_size_ = kMaxSize + sizeof(Header);
    char buffer_[max_block_size_];
};

class Client {
public:
    Client(evpp::EventLoop* loop,
           const std::string& name,
           const std::string& serverAddr, // ip:port
           int sessionCount,
           int total_count,
           int threadCount)
        : loop_(loop),
        name_(name),
        session_count_(sessionCount),
        total_count_(total_count),
        connected_count_(0) {
        tpool_.reset(new evpp::EventLoopThreadPool(loop, threadCount));
        tpool_->Start(true);

        for (int i = 0; i < sessionCount; ++i) {
            std::string n = std::to_string(i);
            Session* session = new Session(tpool_->GetNextLoop(), serverAddr, n, total_count_, this);
            session->Start();
            sessions_.push_back(std::shared_ptr<Session>(session));
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

            uint32_t finished_count = 0;
            uint32_t error_count = 0;
            evpp::Duration total_time;
            for (auto& session : sessions_) {
                if (session->finished()) {
                    ++finished_count;
                    total_time += session->use_time();
                } else {
                    ++error_count;
                }
            }

            LOG_WARN << "name=" << name_ << " error count " << error_count;
            LOG_WARN << "name=" << name_ << " average time(us) " << total_time.Microseconds()/finished_count;
            loop_->QueueInLoop(std::bind(&Client::Quit, this));
        }
    }

private:
    void Quit() {
        tpool_->Stop();
        loop_->Stop();
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
    int total_count_;
    std::vector<std::shared_ptr<Session>> sessions_;
    std::string message_;
    std::atomic<int> connected_count_;
};

void Session::OnConnection(const evpp::TCPConnPtr& conn) {
    if (conn->IsConnected()) {
        conn->SetTCPNoDelay(true);
        conn->ReserveInputBuffer(kMaxSize << 2);
        conn->ReserveOutputBuffer(kMaxSize << 2);
        owner_->OnConnect();
        start_time_ = evpp::Timestamp::Now();

        Header* header = reinterpret_cast<Header*>(buffer_);
        header->body_size_ = htonl(get_body_len());
        header->packet_count_ = htonl(1);

        conn->Send(buffer_, header->get_full_size());
    } else {
        if (stop_time_.IsEpoch()) {
            // server close the connection.
            stop_time_ = evpp::Timestamp::Now();
            finished_ = true;
        }
        owner_->OnDisconnect(conn);
    }
}

int main(int argc, char* argv[]) {
    google::InitGoogleLogging(argv[0]);
    FLAGS_stderrthreshold = 0;
    if (argc != 6) {
        fprintf(stderr, "Usage: client <host_ip> <port> <threads> <total_count> <sessions>\n");
        return -1;
    }

    const char* ip = argv[1];
    uint16_t port = static_cast<uint16_t>(atoi(argv[2]));
    int threadCount = atoi(argv[3]);
    int total_count = atoi(argv[4]);
    int sessionCount = atoi(argv[5]);

    evpp::EventLoop loop;
    std::string serverAddr = std::string(ip) + ":" + std::to_string(port);

    Client client(&loop, argv[0], serverAddr, sessionCount, total_count, threadCount);
    loop.Run();
    return 0;
}




#include "../../../examples/echo/tcpecho/winmain-inl.h"





