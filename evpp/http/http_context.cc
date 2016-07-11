#include "http_context.h"
#include "embedded_http_server.h"
#include "evpp/libevent_headers.h"

namespace evpp {
    namespace http {
        static const std::string __s_null = "";
        HTTPContext::HTTPContext(struct evhttp_request* r)
            : req(r), params(NULL) {
            remote_ip = req->remote_host;
        }

        HTTPContext::~HTTPContext() {
            if (this->params) {
                delete this->params;
                this->params = NULL;
            }
        }

        bool HTTPContext::Init(HTTPService* hsrv) {
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

            return true;
        }

        const char* HTTPContext::original_uri() const {
            return req->uri;
        }

        void HTTPContext::AddResponseHeader(const std::string& key, const std::string& value) {
            evhttp_add_header(req->output_headers, key.data(), value.data());
        }

        const char* HTTPContext::FindRequestHeader(const char* key) {
            return evhttp_find_header(req->input_headers, key);
        }
    }
}