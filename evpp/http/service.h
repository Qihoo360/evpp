#pragma once

#include "evpp/inner_pre.h"
#include "context.h"

struct evhttp;
struct evhttp_bound_socket;
#if defined(EVPP_HTTP_SERVER_SUPPORTS_SSL)
#include <event2/bufferevent_ssl.h>
#include <openssl/ssl.h>
#endif
namespace evpp {
class EventLoop;
class PipeEventWatcher;
namespace http {

// A service can not run itself, it must be attached into one EventLoop
// So we can embed this Service to the existing EventLoop
class EVPP_EXPORT Service {
public:
#if defined(EVPP_HTTP_SERVER_SUPPORTS_SSL)
	Service(EventLoop* l, bool enable_ssl = false,
			const char* certificate_chain_file = "", const char* private_key_file = "");
#else
    Service(EventLoop* loop);
#endif
    ~Service();

    bool Listen(int port);
    void Stop();
    void Pause();
    void Continue();

    // @Note The URI must not hold any parameters
    // @param uri - The URI of the request without any parameters
    void RegisterHandler(const std::string& uri, HTTPRequestCallback callback);

    void RegisterDefaultHandler(HTTPRequestCallback callback);

    EventLoop* loop() const {
        return listen_loop_;
    }

    int port() const {
        return port_;
    }

#if defined(EVPP_HTTP_SERVER_SUPPORTS_SSL)
	bool enable_ssl() const {
		return enable_ssl_;
	}
	const std::string& certificate_chain_file() const {
		return certificate_chain_file_;
	}
	const std::string& private_key_file() const {
		return private_key_file_;
	}
	void set_certificate_chain_file( const std::string& filename) {
		certificate_chain_file_ = filename;
	}
	void set_private_key_file(const std::string& filename) {
		private_key_file_ = filename;
	}
	/* berif 初始化SSL
	 * param force_enable 强制启用SSL
	 */
	bool initSSL(bool force_enable = false);
#endif					
private:
    static void GenericCallback(struct evhttp_request* req, void* arg);
    void HandleRequest(struct evhttp_request* req);
    void DefaultHandleRequest(const ContextPtr& ctx);
    void SendReply(const ContextPtr& ctx, const std::string& response);
private:
    int port_ = 0;
    struct evhttp* evhttp_;
    struct evhttp_bound_socket* evhttp_bound_socket_;
    EventLoop* listen_loop_;
    HTTPRequestCallbackMap callbacks_;
    HTTPRequestCallback default_callback_;

	// HTTPS 支持
#if defined(EVPP_HTTP_SERVER_SUPPORTS_SSL)
	bool enable_ssl_;
	SSL_CTX* ssl_ctx_;
	std::string certificate_chain_file_;
	std::string private_key_file_;
#endif
};
}

}
