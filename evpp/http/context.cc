#include "context.h"
#include "service.h"
#include "evpp/libevent.h"
#include "evpp/memmem.h"

namespace evpp {
namespace http {
Context::Context(struct evhttp_request* r)
    : req_(r) {
}

Context::~Context() {
}

bool Context::Init() {
    if (req_->type == EVHTTP_REQ_POST) {
#if LIBEVENT_VERSION_NUMBER >= 0x02001500
        struct evbuffer* evbuf = evhttp_request_get_input_buffer(req_);
        size_t buffer_size = evbuffer_get_length(evbuf);
        if (buffer_size > 0) {
            this->body_ = Slice((char*)evbuffer_pullup(evbuf, -1), buffer_size);
        }
#else
        if (req_->input_buffer->off > 0) {
            this->body_ = Slice((char*)req_->input_buffer->buffer, req_->input_buffer->off);
        }
#endif
    }

#if LIBEVENT_VERSION_NUMBER >= 0x02001500
    uri_ = evhttp_uri_get_path(req_->uri_elems);
#else
    const char* p = strchr(req_->uri, '?');
    if (p != nullptr) {
        uri_ = std::string(req_->uri, p - req_->uri);
    } else {
        uri_ = req_->uri;
    }

#endif

    remote_ip_ = FindClientIP(original_uri());
    if (remote_ip_.empty()) {
        remote_ip_ = req_->remote_host;
    }

    return true;
}

const char* Context::original_uri() const {
    return req_->uri;
}

void Context::AddResponseHeader(const std::string& key, const std::string& value) {
    evhttp_add_header(req_->output_headers, key.data(), value.data());
}

const char* Context::FindRequestHeader(const char* key) {
    return evhttp_find_header(req_->input_headers, key);
}

std::string Context::FindClientIP(const char* uri) {
    static const std::string __s_nullptr = "";
    static const std::string __s_clientip = "clientip=";
    const char* found = static_cast<const char*>(memmem(uri, strlen(uri), __s_clientip.data(), __s_clientip.size()));
    if (found) {
        const char* ip = found + __s_clientip.size();
        const char* end = strchr(const_cast<char*>(ip), '&');
        if (!end) {
            return ip;
        } else {
            return std::string(ip, end);
        }
    }

    return __s_nullptr;
}
}
}
