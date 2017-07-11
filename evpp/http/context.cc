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
    const char* original_url = original_uri();
    remote_ip_ = FindClientIPFromURI(original_url, strlen(original_url));
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

std::string Context::FindQueryFromURI(const char* uri, size_t uri_len, const char* key, size_t key_len) {
    static const std::string __s_nullptr = "";

    // Find query start point
    const char* start = strchr(const_cast<char*>(uri), '?');
    if (!start) {
        return __s_nullptr;
    }

    for (const char* p = start + 1; p < uri + uri_len;) {
        size_t index = 0;
        for (; index < key_len; ++index) {
            if (p[index] != key[index]) {
                break;
            }
        }

        if (index == key_len && p[key_len] == '=') {
            // Found the key
            const char* v = p + key_len + 1;
            const char* end = strchr(const_cast<char*>(v), '&');
            if (!end) {
                return v;
            } else {
                return std::string(v, end);
            }
        }

        // Skip to next query
        p += index;
        p = strchr(const_cast<char*>(p), '&');
        if (!p) {
            return __s_nullptr;
        }
        p += 1;
    }

    return __s_nullptr;
}

std::string Context::FindQueryFromURI(const char* uri, const char* key) {
    return FindQueryFromURI(uri, strlen(uri), key, strlen(key));
}

std::string Context::FindQueryFromURI(const std::string& uri, const std::string& key) {
    return FindQueryFromURI(uri.data(), uri.size(), key.data(), key.size());
}

}
}
