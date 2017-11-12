
#include "evpp/libevent.h"
#include "evpp/httpc/conn.h"
#include "evpp/httpc/conn_pool.h"
#include "evpp/httpc/response.h"
#include "evpp/httpc/request.h"

namespace evpp {
namespace httpc {
#if defined(EVPP_HTTP_CLIENT_SUPPORTS_SSL)
Response::Response(Request* r, struct evhttp_request* evreq, bool had_ssl_error)
    : request_(r), evreq_(evreq), http_code_(0), had_ssl_error_(had_ssl_error) {
#else
Response::Response(Request* r, struct evhttp_request* evreq)
    : request_(r), evreq_(evreq), http_code_(0) {
#endif
    if (evreq) {
        http_code_ = evreq->response_code;

#if LIBEVENT_VERSION_NUMBER >= 0x02001500
        struct evbuffer* evbuf = evhttp_request_get_input_buffer(evreq);
        size_t buffer_size = evbuffer_get_length(evbuf);
        if (buffer_size > 0) {
            this->body_ = evpp::Slice((char*)evbuffer_pullup(evbuf, -1), buffer_size);
        }
#else
        if (evreq->input_buffer->off > 0) {
            this->body_ = evpp::Slice((char*)evreq->input_buffer->buffer, evreq->input_buffer->off);
        }
#endif
    }
}

Response::~Response() {
}

const char* Response::FindHeader(const char* key) {
    if (http_code_ > 0) {
        assert(this->evreq_);
        return evhttp_find_header(this->evreq_->input_headers, key);
    }
    return nullptr;
}

}
}


