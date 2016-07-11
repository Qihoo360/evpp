#include "service.h"

#include "evpp/libevent_headers.h"
#include "evpp/libevent_watcher.h"
#include "evpp/event_loop.h"

namespace evpp {
    namespace http {
        Service::Service(EventLoop* l)
            : evhttp_(NULL), loop_(l) {
            evhttp_ = evhttp_new(loop_->event_base());
            if (!evhttp_) {
                return;
            }
        }

        Service::~Service() {
            assert(!evhttp_);
            assert(!loop_);
        }

        bool Service::Listen(int port) {
            assert(evhttp_);
            assert(loop_->IsInLoopThread());
            if (evhttp_bind_socket(evhttp_, "0.0.0.0", port) != 0) {
                return false;
            }

            evhttp_set_gencb(evhttp_, &Service::GenericCallback, this);
            return true;
        }

        void Service::Stop() {
            assert(!loop_->running() || loop_->IsInLoopThread());
            if (evhttp_) {
                evhttp_free(evhttp_);
                evhttp_ = NULL;
            }
            loop_ = NULL;
            callbacks_.clear();
            default_callback_ = HTTPRequestCallback();
        }

        bool Service::RegisterEvent(const std::string& uri, HTTPRequestCallback callback) {
            callbacks_[uri] = callback;
            return true;
        }

        bool Service::RegisterDefaultEvent(HTTPRequestCallback callback) {
            default_callback_ = callback;
            return true;
        }

        void Service::GenericCallback(struct evhttp_request *req, void *arg) {
            Service* hsrv = (Service*)arg;
            hsrv->HandleRequest(req);
        }

        void Service::HandleRequest(struct evhttp_request *req) {
            // HTTP 请求的处理总入口，在监听主线程中执行
            assert(loop_->IsInLoopThread());
            LOG_TRACE << "handle request " << req << " url=" << req->uri;

            HTTPContextPtr ctx(new HTTPContext(req));
            ctx->Init(this);

            HTTPRequestCallbackMap::iterator it = callbacks_.find(ctx->uri);
            if (it == callbacks_.end()) {
                DefaultHandleRequest(ctx);
                return;
            }

            // 此处会调度到 HTTPServer::Dispatch 函数中
            it->second(ctx, std::bind(&Service::SendReply, this, req, std::placeholders::_1));
        }

        void Service::DefaultHandleRequest(const HTTPContextPtr& ctx) {
            if (default_callback_) {
                default_callback_(ctx, std::bind(&Service::SendReply, this, ctx->req, std::placeholders::_1));
            } else {
                evhttp_send_reply(ctx->req, HTTP_BADREQUEST, "Bad Request", NULL);
            }
        }

        struct Response {
            Response(struct evhttp_request* r, const std::string& m)
                : req(r), buffer(NULL) {
                if (m.size() > 0) {
                    buffer = evbuffer_new();
                    evbuffer_add(buffer, m.c_str(), m.size());
                }
            }

            ~Response() {
                if (buffer) {
                    evbuffer_free(buffer);
                    buffer = NULL;
                }
                LOG_TRACE << "free request " << req->uri;
            }

            struct evhttp_request*  req;
            struct evbuffer *buffer;
        };

        void Service::SendReply(struct evhttp_request *req, const std::string& response) {
            // 在工作线程中执行，将HTTP响应包的发送权交还给监听主线程
            LOG_TRACE << "send reply in working thread";

            // 在工作线程中准备好响应报文
            std::shared_ptr<Response> pt(new Response(req, response));
            
            auto f = [this](const std::shared_ptr<Response>& pt) {
                assert(this->loop_->IsInLoopThread());
                LOG_TRACE << "send reply";
                if (!pt->buffer) {
                    evhttp_send_reply(pt->req, HTTP_NOTFOUND, "Not Found", NULL);
                    return;
                }

                evhttp_send_reply(pt->req, HTTP_OK, "OK", pt->buffer);
            };

            loop_->RunInLoop(std::bind(f, pt));
        }
    }
}
