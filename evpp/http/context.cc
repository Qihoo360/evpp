#include "context.h"
#include "service.h"
#include "evpp/libevent_headers.h"
#include "evpp/memmem.h"

namespace evpp {
namespace http {
Context::Context(struct evhttp_request* r, EventLoop* l)
    : req(r), dispatched_loop(l) {
}

Context::~Context() {
}

bool Context::Init(Service* hsrv) {
    if (req->type == EVHTTP_REQ_POST) {
#if LIBEVENT_VERSION_NUMBER >= 0x02001500
        struct evbuffer* evbuf = evhttp_request_get_input_buffer(req);
        size_t buffer_size = evbuffer_get_length(evbuf);
        if (buffer_size > 0) {
            this->body = Slice((char*)evbuffer_pullup(evbuf, -1), buffer_size);
        }
#else
        if (req->input_buffer->off > 0) {
            this->body = Slice((char*)req->input_buffer->buffer, req->input_buffer->off);
        }
#endif
    }

#if LIBEVENT_VERSION_NUMBER >= 0x02001500
    uri = evhttp_uri_get_path(req->uri_elems);
#else
    const char* p = strchr(req->uri, '?');
    if (p != NULL) {
        uri = std::string(req->uri, p - req->uri);
    } else {
        uri = req->uri;
    }

#endif

    remote_ip = FindClientIP(original_uri());
    if (remote_ip.empty()) {
        remote_ip = req->remote_host;
    }

    return true;
}

const char* Context::original_uri() const {
    return req->uri;
}

void Context::AddResponseHeader(const std::string& key, const std::string& value) {
    evhttp_add_header(req->output_headers, key.data(), value.data());
}

const char* Context::FindRequestHeader(const char* key) {
    return evhttp_find_header(req->input_headers, key);
}

std::string Context::FindClientIP(const char* uri) {
    static const std::string __s_null = "";
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

    return __s_null;
}
}
}