
#include "evpp/libevent_headers.h"
#include "evpp/httpc/conn.h"
#include "evpp/httpc/pool.h"
#include "evpp/httpc/response.h"

namespace evpp {
    namespace httpc {
        Response::Response(struct evhttp_request* r) 
            : evhttp_request_(r), http_code_(r->response_code) {
#if LIBEVENT_VERSION_NUMBER >= 0x02001500
            struct evbuffer* evbuf = evhttp_request_get_input_buffer(r);
            size_t buffer_size = evbuffer_get_length(evbuf);
            if (buffer_size > 0) {
                this->body_ = evpp::Slice((char*)evbuffer_pullup(evbuf, -1), buffer_size);
            }
#else
            if (r->input_buffer->off > 0) {
                this->body_ = evpp::Slice((char*)r->input_buffer->buffer, r->input_buffer->off);
            }
#endif

//             struct evkeyval *header;
//             TAILQ_FOREACH(header, r->input_headers, next) {
//                 headers_[header->key] = header->value;
//             }
        }

        Response::~Response() {

        }

    } // httpc
} // evpp


