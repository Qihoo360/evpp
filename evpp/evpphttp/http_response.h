#pragma once
#include <algorithm>
#include <evpp/buffer.h>
#include <evpp/tcp_conn.h>
#include <evpp/evpphttp/http_request.h>
#include <evpp/evpphttp/http_parser.h>
#include <cctype>
namespace evpp {
namespace evpphttp {
static std::map<int, std::string> http_status_code = {
    {100, "Continue"},
    {101, "Switching Protocols"},
    {200, "OK"},
    {201, "Created"},
    {202, "Accepted"},
    {203, "Non-Authoritative Information"},
    {204, "No Content"},
    {205, "Reset Content"},
    {206,  "Partial Content"},
    {300, "Multiple Choices"},
    {301, "Moved Permanently"},
    {302, "Found"},
    {303, "See Other"},
    {304, "Not Modified"},
    {305, "Use Proxy"},
    {307, "Temporary Redirect"},
    {400, "Bad Request"},
    {401, "Unauthorized"},
    {402, "Payment Required"},
    {403, "Forbidden"},
    {404, "Not Found"},
    {405, "Method Not Allowed"},
    {406, "Not Acceptable"},
    {407, "Proxy Authentication Required"},
    {408, "Request Time-out"},
    {409, "Conflict"},
    {410, "Gone"},
    {411, "Length Required"},
    {412, "Precondition Failed"},
    {413, "Request Entity Too Large"},
    {414, "Request-URI Too Large"},
    {415, "Unsupported Media Type"},
    {416, "Requested range not satisfiable"},
    {417, "Expectation Failed"},
    {500, "Internal Server Error"},
    {501, "Not Implemented"},
    {502, "Bad Gateway"},
    {503, "Service Unavailable"},
    {504, "Gateway Time-out"},
    {505,  "HTTP Version not supported"},
    {808,  "UnKnown"}  //private
};
#define is_connection_xx(name, key) \
	bool is_connection_##name(const std::map<std::string, std::string>& field_value) { \
		auto iter = field_value.find("Connection");\
		if (iter != field_value.end()) { \
			auto data = iter->second; \
			std::transform(data.begin(), data.end(), data.begin(), ::tolower);\
			if (data.compare(#key) == 0) { \
				return true; \
			} \
		} \
		return false; \
	}

class HttpResponse {
public:
    HttpResponse(const HttpRequest& hr);
    HttpResponse(const HttpResponse& other) : close_(other.close_), keep_alive_(other.keep_alive_), chunked_(other.chunked_), hp_(other.hp_) {}
    void SendReply(const evpp::TCPConnPtr& conn, const int response_code, const std::map<std::string, std::string>& header_field_value, const std::string & response_body);
    void MakeHttpResponse(const int response_code, const int64_t body_size, const std::map<std::string, std::string>& header_field_value, Buffer& buf);

private:
    is_connection_xx(close, close);
    is_connection_xx(keep_alive, keep-alive);
    void add_content_len(const int64_t size, Buffer& buf);
    void add_date(Buffer& buf);
    void SendContinue(const evpp::TCPConnPtr& conn);
    inline bool need_body(const int response_code) {
        return (response_code != 204 && response_code != 304 && (response_code < 100 || response_code >= 200));
    }
private:
    bool close_;
    bool keep_alive_;
    bool chunked_{false};
    http_parser hp_;
};
}
}

