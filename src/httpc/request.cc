
#include "evpp/libevent_headers.h"
#include "evpp/httpc/conn.h"
#include "evpp/httpc/pool.h"
#include "evpp/httpc/response.h"
#include "evpp/httpc/request.h"

namespace evpp {
    namespace httpc {
        Request::Request(Pool* pool, EventLoop* loop, const std::string& uri, const std::string& body) 
            : pool_(pool), loop_(loop), uri_(uri), body_(body) {

        }

        Request::~Request() {

        }

        void Request::Execute(const Handler& h) {
            if (loop_->IsInLoopThread()) {
                ExecuteInLoop(h);
            } else {
                loop_->RunInLoop(std::bind(&Request::ExecuteInLoop, this, h));
            }
        }
        
        void Request::ExecuteInLoop(const Handler& h) {
            handler_ = h;

            std::string errmsg;
            struct evhttp_request *req = NULL;
            std::shared_ptr<Conn> conn = pool_->Get(loop_);
            if (!conn->Init()) {
                errmsg = "conn init fail";
                goto failed;
            }
            conn_ = conn;

            req = evhttp_request_new(&Request::HandleResponse, this);
            if (!req) {
                errmsg = "evhttp_request_new fail";
                goto failed;
            }

            if (evhttp_add_header(req->output_headers, "host", pool_->host().c_str())) {
                evhttp_request_free(req);
                errmsg = "evhttp_add_header failed";
                goto failed;
            }

            evhttp_cmd_type req_type = EVHTTP_REQ_GET;
            if (!body_.empty()) {
                req_type = EVHTTP_REQ_POST;
                if (evbuffer_add(req->output_buffer, body_.c_str(), body_.size())) {
                    evhttp_request_free(req);
                    errmsg = "evbuffer_add fail";
                    goto failed;
                }
            }

            if (evhttp_make_request(conn->evhttp_conn(), req, req_type, uri_.c_str())) {
                // here the conn has own the req, so don't free it twice.
                errmsg = "evhttp_make_request fail";
                goto failed;
            }
            return;

        failed:
            LOG_ERROR << "http request failed: " << errmsg;
            std::shared_ptr<Response> response;
            handler_(response);
        }

        void Request::HandleResponse(struct evhttp_request* rsp, void *v) {
            Request* thiz = (Request*)v;
            assert(thiz);

            //ErrCode ec = OK;
            std::shared_ptr<Response> response;
            if (rsp) {
                //ec = OK;
                response.reset(new Response(rsp));
                thiz->pool_->Put(thiz->conn_);
            } else {
                //ec = E_CONN_FAILED;
            }
            thiz->handler_(response);
        }
    } // httpc
} // evpp


