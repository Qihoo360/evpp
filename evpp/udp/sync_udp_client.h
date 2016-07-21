#include "evpp/inner_pre.h"

#include "udp_message.h"

namespace evpp {
namespace udp {
namespace sync {
class EVPP_EXPORT Client {
public:
    //! Constructors
    //! \param sockfd, 
    //!     If it equals to INVALID_SOCKET, you need to call <code>connect(...)</code> method to connect to UDP server
    //!     else you can use this instance to receive and send data directly.
    Client();
    ~Client();

    bool Connect(const char* host, int port);
    bool Connect(const char* addr/*host:port*/);
    bool Connect(const struct sockaddr& addr);
    bool Connect(const struct sockaddr_in& addr);

    //! Disconnect the pipe from remote side.
    //! \remark It is usually called by application user when want to disconnect the pipe.
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
public:
    int sockfd() const {
        return sockfd_;
    }
private:
    bool Connect();
    int sockfd_;
    struct sockaddr remote_addr_;
};
}
}
}