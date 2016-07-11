#include "embedded_http_server.h"
#include "evpp/libevent_headers.h"
#include "evpp/libevent_watcher.h"

namespace evpp {
    namespace http {
        HTTPService::PendingReply::PendingReply(struct evhttp_request* r, const std::string& m) : req(r), buffer(NULL) {
            if (m.size() > 0) {
                buffer = evbuffer_new();
                evbuffer_add(buffer, m.c_str(), m.size());
            }
        }

        HTTPService::PendingReply::~PendingReply() {
            if (buffer) {
                evbuffer_free(buffer);
                buffer = NULL;
            }
            LOG_TRACE << "free request " << req->uri;
        }


        HTTPService::HTTPService(struct event_base* base /*= NULL*/)
            : evhttp_(NULL), event_base_(base) {
            if (!event_base_) {
                return;
            }

            evhttp_ = evhttp_new(event_base_);
            if (!evhttp_) {
                return;
            }
        }

        HTTPService::~HTTPService() {
            Stop();
        }

        bool HTTPService::Listen(int port) {
            if (!evhttp_ || !event_base_) {
                return false;
            }

            if (evhttp_bind_socket(evhttp_, "0.0.0.0", port) != 0) {
                return false;
            }

            pending_reply_list_watcher_.reset(new PipeEventWatcher(event_base_, std::bind(&HTTPService::HandleReply, this)));
            pending_reply_list_watcher_->Init();
            pending_reply_list_watcher_->AsyncWait();

            evhttp_set_gencb(evhttp_, &HTTPService::GenericCallback, this);

            return true;
        }

        void HTTPService::Stop() {
            if (evhttp_) {
                evhttp_free(evhttp_);
                evhttp_ = NULL;
            }
            event_base_ = NULL;
            callbacks_.clear();
            default_callback_ = HTTPRequestCallback();
        }

        bool HTTPService::RegisterEvent(const std::string& uri, HTTPRequestCallback callback) {
            callbacks_[uri] = callback;
            return true;
        }

        bool HTTPService::RegisterDefaultEvent(HTTPRequestCallback callback) {
            default_callback_ = callback;
            return true;
        }

        void HTTPService::GenericCallback(struct evhttp_request *req, void *arg) {
            HTTPService* hsrv = (HTTPService*)arg;
            hsrv->HandleRequest(req);
        }

        void HTTPService::HandleRequest(struct evhttp_request *req) {
            LOG_TRACE << "handle request " << req << " url=" << req->uri;

            HTTPContextPtr ctx(new HTTPContext(req));
            ctx->Init(this);

            HTTPRequestCallbackMap::iterator it = callbacks_.find(ctx->uri);
            if (it == callbacks_.end()) {
                DefaultHandleRequest(ctx);
                return;
            }

            it->second(ctx, std::bind(&HTTPService::SendReplyT, this, req, std::placeholders::_1));
        }

        void HTTPService::DefaultHandleRequest(const HTTPContextPtr& ctx) {
            if (default_callback_) {
                default_callback_(ctx, std::bind(&HTTPService::SendReplyT, this, ctx->req, std::placeholders::_1));
            } else {
                evhttp_send_reply(ctx->req, HTTP_BADREQUEST, "Bad Request", NULL);
            }
        }

        void HTTPService::SendReplyInLoop(const PendingReplyPtr& pt) {
            LOG_TRACE << "send reply";
            if (!pt->buffer) {
                evhttp_send_reply(pt->req, HTTP_NOTFOUND, "Not Found", NULL);
                return;
            }

            evhttp_send_reply(pt->req, HTTP_OK, "OK", pt->buffer);
        }

        void HTTPService::SendReplyT(struct evhttp_request *req, const std::string& response) {
            LOG_TRACE << "send reply in working thread";
            PendingReplyPtr pt(new PendingReply(req, response));
            
            std::lock_guard<std::mutex> guard(pending_reply_list_lock_);
            pending_reply_list_.push_back(pt);
            pending_reply_list_watcher_->Notify();
        }

        void HTTPService::HandleReply() {
            // If http server is stopping or stopped
            if (!evhttp_) {
                return;
            }

            PendingReplyPtrList reply_list;

            {
                std::lock_guard<std::mutex> guard(pending_reply_list_lock_);
                reply_list.swap(pending_reply_list_);
            }

            PendingReplyPtrList::iterator it(reply_list.begin());
            PendingReplyPtrList::iterator ite(reply_list.end());
            for (; it != ite; ++it) {
                PendingReplyPtr& pt = *it;
                SendReplyInLoop(pt);
            }

            reply_list.clear();
        }

    }
}
