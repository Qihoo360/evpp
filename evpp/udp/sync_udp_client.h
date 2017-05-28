#include "evpp/inner_pre.h"

#include "udp_message.h"

namespace evpp {
namespace udp {
namespace sync {

// It is not asynchronous, please do not use it production.
// The only purpose it exists is for purpose of testing UDP Server.
class EVPP_EXPORT Client {
public:
    Client();
    ~Client();

    bool Connect(const char* host, int port);
    bool Connect(const char* addr/*host:port*/);
    bool Connect(const struct sockaddr_storage& addr);
    bool Connect(const struct sockaddr& addr);
    bool Connect(const struct sockaddr_in& addr);

    void Close();

    bool Send(const std::string& msg);
    bool Send(const char* msg, size_t len);

    //! brief : Do a udp request and wait for remote udp server send response data
    //! param[in] - const std::string & udp_package_data
    //! return - std::string the response data
    std::string DoRequest(const std::string& udp_package_data, uint32_t timeout_ms);

    static std::string DoRequest(const std::string& remote_ip, int port, const std::string& udp_package_data, uint32_t timeout_ms);

    static bool Send(const std::string& msg, const struct sockaddr_in& addr);
    static bool Send(const char* msg, size_t len, const struct sockaddr_in& addr);
    static bool Send(const MessagePtr& msg);
    static bool Send(const Message* msg);
public:
    evpp_socket_t sockfd() const {
        return sockfd_;
    }
private:
    bool Connect();
    int sockfd_;
    struct sockaddr_storage remote_addr_;
};
}
}
}