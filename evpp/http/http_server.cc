#include "http_server.h"

#include "evpp/libevent_headers.h"
#include "evpp/libevent_watcher.h"
#include "evpp/event_loop.h"
#include "evpp/event_loop_thread.h"
#include "evpp/event_loop_thread_pool.h"

namespace evpp {
namespace http {
HTTPServer::HTTPServer(uint32_t thread_num) : threads_dispatch_policy_(kRoundRobin) {
    listen_thread_.reset(new EventLoopThread());
    tpool_.reset(new EventLoopThreadPool(listen_thread_->event_loop(), thread_num));
    http_.reset(new Service(listen_thread_->event_loop()));
}

HTTPServer::~HTTPServer() {
    http_.reset();
    listen_thread_.reset();
    tpool_.reset();
}

bool HTTPServer::Start(int port) {
    if (!http_->Listen(port)) {
        return false;
    }

    bool rc = tpool_->Start(true);

    if (!rc) {
        return rc;
    }

    listen_thread_->SetName("StandaloneHTTPServer-Main");

    // �� listen_thread_ �˳�ʱ������øú�����ֹͣ Service
    auto http_close_fn = std::bind(&Service::Stop, http_);
    rc = listen_thread_->Start(true,
                               EventLoopThread::Functor(),
                               http_close_fn);

    while (!IsRunning()) {
        usleep(1);
    }

    return rc;
}

void HTTPServer::Stop(bool wait_thread_exit /*= false*/) {
    listen_thread_->Stop();
    tpool_->Stop();

    // http_ �����Stop���� listen_thread_ �˳�ʱ�Զ�ִ�� Service::Stop

    if (wait_thread_exit) {
        while (!listen_thread_->IsStopped() || !tpool_->IsStopped()) {
            usleep(1);
        }
    }
}

bool HTTPServer::IsRunning() const {
    return listen_thread_->IsRunning() && tpool_->IsRunning();
}

bool HTTPServer::IsStopped() const {
    return listen_thread_->IsStopped() && tpool_->IsStopped();
}

bool HTTPServer::RegisterHandler(const std::string& uri, HTTPRequestCallback callback) {
    HTTPRequestCallback cb = std::bind(&HTTPServer::Dispatch, this,
                                       std::placeholders::_1,
                                       std::placeholders::_2,
                                       callback);
    return http_->RegisterHandler(uri, cb);
}

bool HTTPServer::RegisterDefaultHandler(HTTPRequestCallback callback) {
    HTTPRequestCallback cb = std::bind(&HTTPServer::Dispatch, this,
                                       std::placeholders::_1,
                                       std::placeholders::_2,
                                       callback);
    return http_->RegisterDefaultHandler(cb);
}

void HTTPServer::Dispatch(const ContextPtr& ctx,
                          const HTTPSendResponseCallback& response_callback,
                          const HTTPRequestCallback& user_callback) {
    LOG_TRACE << "dispatch request " << ctx->req << " url=" << ctx->original_uri() << " in main thread";

    // ����HTTP������ȵ������̴߳���
    auto f = [](const ContextPtr & context,
                const HTTPSendResponseCallback & response_cb,
    const HTTPRequestCallback & user_cb) {
        LOG_TRACE << "process request " << context->req
                  << " url=" << context->original_uri() << " in working thread";

        // �ڹ����߳���ִ�У������ϲ�Ӧ�����õĻص������������HTTP����
        // ���ϲ�Ӧ�ô�����󣬱���Ҫ���� response_callback ������������������Ҳ���ǻ�ص� Service::SendReply �����С�
        user_cb(context, response_cb);
    };
    EventLoop* loop = NULL;
    if (threads_dispatch_policy_ == kRoundRobin) {
        loop = tpool_->GetNextLoop();
    } else {
        uint64_t hash = std::hash<std::string>()(ctx->remote_ip);
        loop = tpool_->GetNextLoopWithHash(hash);
    }
    loop->RunInLoop(std::bind(f, ctx, response_callback, user_callback));
}

Service* HTTPServer::service() const {
    return http_.get();
}
}
}
