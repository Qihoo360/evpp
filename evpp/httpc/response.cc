
#include "evpp/libevent_headers.h"
#include "evpp/httpc/conn.h"
#include "evpp/httpc/conn_pool.h"
#include "evpp/httpc/response.h"
#include "evpp/httpc/request.h"

namespace evpp {
    namespace httpc {
        Response::Response(Request* r, struct evhttp_request* evreq)
            : request_(r), evreq_(evreq), http_code_(evreq->response_code) {
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

//             struct evkeyval *header;
//             TAILQ_FOREACH(header, r->input_headers, next) {
//                 headers_[header->key] = header->value;
//             }
        }


        Response::Response(Request* r)
            : request_(r), evreq_(NULL), http_code_(0) {
        }

        Response::~Response() {

        }

    } // httpc
} // evpp


