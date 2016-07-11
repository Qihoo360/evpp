#pragma once

#include "evpp/inner_pre.h"
#include "http_context.h"

#include <vector>
#include <list>
#include <mutex>

struct event;
struct event_base;

namespace evpp {
    class PipeEventWatcher;
    namespace https {
        class EVPP_EXPORT HTTPService {
        public:
            HTTPService(struct event_base* base);
            ~HTTPService();

            bool Listen(int port);

            void Stop();

            //! \brief URI=/uri?q=value&k=v
            //!     @see unit test <code>http_ParseURI_test</code>
            //! \param[in] - const std::string & uri
            //! \param[in] - RequestCallback callback
            //! \return - bool
            bool RegisterEvent(const std::string& uri, HTTPRequestCallback callback);

            bool RegisterDefaultEvent(HTTPRequestCallback callback);

            void AddRequestHeaderKeyForParsing(const std::string& key) {
                request_header_keys_for_parsing_.insert(key);
            }

            const stringset& request_header_keys_for_parsing() const {
                return request_header_keys_for_parsing_;
            }

        private:
            void HandleRequest(struct evhttp_request *req);

            void DefaultHandleRequest(const HTTPContextPtr& ctx);

        private:
            static void GenericCallback(struct evhttp_request *req, void *arg);

        private:
            //thread-safe, it will be called from other thread possibly
            void SendReplyT(struct evhttp_request *req, const std::string& response);

            void HandleReply();

        private:
            struct evhttp*      evhttp_;
            struct event_base*  event_base_;

            stringset request_header_keys_for_parsing_;

            HTTPRequestCallbackMap  callbacks_;
            HTTPRequestCallback     default_callback_;
        private:
            struct PendingReply {
                PendingReply(struct evhttp_request* r, const std::string& m);
                ~PendingReply();

                struct evhttp_request*  req;
                struct evbuffer *buffer;
            };

            typedef std::shared_ptr<PendingReply> PendingReplyPtr;

            typedef std::list<PendingReplyPtr> PendingReplyPtrList;
            PendingReplyPtrList         pending_reply_list_;
            std::mutex                  pending_reply_list_lock_;
            std::shared_ptr<PipeEventWatcher> pending_reply_list_watcher_;

        private:
            void SendReplyInLoop(const PendingReplyPtr& pt);
        };
    }

}