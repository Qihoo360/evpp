#include "http_context.h"
#include "embedded_http_server.h"
#include "evpp/libevent_headers.h"

namespace evpp {
    namespace https {
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

#if 0
            //TODO refactor this code
            const stringset& request_header_keys_for_parsing = hsrv->request_header_keys_for_parsing();

            if (hsrv->parse_parameters() || request_header_keys_for_parsing.size() > 0) {
                this->params = new HTTPParameterMap;
            }

            bool rc = osl::URIParser::parse(this->req->uri, strlen(this->req->uri), this->uri, this->params);

            if (this->params) {
                static const std::string __s_clientip = "clientip";
                HTTPParameterMap::const_iterator it = this->params->find(__s_clientip);
                if (it != this->params->end()) {
                    remote_ip = it->second;
                }
            }


            if (request_header_keys_for_parsing.size() > 0) {
                assert(this->params);
                stringset::const_iterator it(request_header_keys_for_parsing.begin());
                stringset::const_iterator ite(request_header_keys_for_parsing.end());
                for (; it != ite; ++it) {
                    const std::string& key = *it;
                    struct evkeyvalq *headers = req->input_headers;
                    const char* value = evhttp_find_header(headers, key.c_str());
                    if (value) {
                        (*this->params)[key] = value;
                    }
                }
            }
            return rc;
#endif

            return true;
        }

        const char* HTTPContext::original_uri() const {
            return req->uri;
        }

        const std::string& HTTPContext::GetRequestHeader(const std::string& key, bool* found) {
            if (this->params) {
                HTTPParameterMap::const_iterator it = this->params->find(key);
                if (it != this->params->end()) {
                    if (found) {
                        *found = true;
                    }
                    return it->second;
                }
            }


            if (found) {
                *found = false;
            }

            return __s_null;
        }

        void HTTPContext::AddResponseHeader(const std::string& key, const std::string& value) {
            evhttp_add_header(req->output_headers, key.data(), value.data());
        }

    }
}