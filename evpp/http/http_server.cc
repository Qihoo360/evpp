#include "http_server.h"

#include "evpp/libevent_headers.h"
#include "evpp/libevent_watcher.h"
#include "evpp/event_loop.h"
#include "evpp/event_loop_thread.h"
#include "evpp/event_loop_thread_pool.h"

namespace evpp {
namespace http {
HTTPServer::HTTPServer(uint32_t thread_num) {
    tpool_.reset(new EventLoopThreadPool(NULL, thread_num));
}

HTTPServer::~HTTPServer() {
//     http_.reset();
//     listen_thread_.reset();
    tpool_.reset();
}

bool HTTPServer::Start(int port) {
    bool rc = tpool_->Start(true);
    if (!rc) {
        return rc;
    }

    rc = rc && StartListenThread(port);

    while (!IsRunning()) {
        usleep(1);
    }

    return rc;
}


bool HTTPServer::Start(std::vector<int> listen_ports) {
    bool rc = tpool_->Start(true);
    if (!rc) {
        return rc;
    }

    for (size_t i = 0; i < listen_ports.size(); ++i) {
        if (!StartListenThread(listen_ports[i])) {
            return false;
        }
    }
    return true;
}

bool HTTPServer::StartListenThread(int port) {
    ListenThread lt;
    lt.t = std::make_shared<EventLoopThread>();
    lt.t->SetName(std::string("StandaloneHTTPServer-Main-") + std::to_string(port));

    lt.h = std::make_shared<Service>(lt.t->event_loop());
    if (!lt.h->Listen(port)) {
        LOG_ERROR << "http server listen at port " << port << " failed.";
        return false;
    }

    // 当 ListenThread 退出时，会调用该函数来停止 Service
    auto http_close_fn = std::bind(&Service::Stop, lt.h);
    bool rc = lt.t->Start(true,
                          EventLoopThread::Functor(),
                          http_close_fn);
    assert(lt.t->IsRunning());
    for (auto it = callbacks_.begin(); it != callbacks_.end(); ++it) {
        lt.h->RegisterHandler(it->first, it->second);
    }
    lt.h->RegisterDefaultHandler(default_callback_);
    listen_threads_.push_back(lt);
    return rc;
}

void HTTPServer::Stop(bool wait_thread_exit /*= false*/) {
    for (auto it = listen_threads_.begin(); it != listen_threads_.end(); ++it) {
        it->t->Stop();
    }
    tpool_->Stop();

    // http_ 对象的Stop会在 listen_thread_ 退出时自动执行 Service::Stop

    if (wait_thread_exit) {
        for (;;) {
            bool stopped = true;
            for (auto it = listen_threads_.begin(); it != listen_threads_.end(); ++it) {
                if (!it->t->IsStopped()) {
                    stopped = false;
                    break;
                }
            }

            if (!tpool_->IsStopped()) {
                stopped = false;
            }

            if (stopped) {
                break;
            }

            usleep(1);
        }
    }
}


void HTTPServer::Pause() {
    for (auto it = listen_threads_.begin(); it != listen_threads_.end(); ++it) {
        EventLoop* loop = it->t->event_loop();
        loop->RunInLoop(std::bind(&Service::Pause, it->h));
    }
}

void HTTPServer::Continue() {
    for (auto it = listen_threads_.begin(); it != listen_threads_.end(); ++it) {
        EventLoop* loop = it->t->event_loop();
        loop->RunInLoop(std::bind(&Service::Continue, it->h));
    }
}

bool HTTPServer::IsRunning() const {
    if (listen_threads_.empty()) {
        return false;
    }

    if (!tpool_->IsRunning()) {
        return false;
    }

    for (auto it = listen_threads_.begin(); it != listen_threads_.end(); ++it) {
        if (!it->t->IsRunning()) {
            return false;
        }
    }
    return true;
}

bool HTTPServer::IsStopped() const {
    assert(!listen_threads_.empty());
    assert(tpool_);

    if (!tpool_->IsStopped()) {
        return false;
    }

    for (auto it = listen_threads_.begin(); it != listen_threads_.end(); ++it) {
        if (!it->t->IsStopped()) {
            return false;
        }
    }
}

void HTTPServer::RegisterHandler(const std::string& uri, HTTPRequestCallback callback) {
    if (IsRunning()) {
        for (auto it = listen_threads_.begin(); it != listen_threads_.end(); ++it) {
            HTTPRequestCallback cb = std::bind(&HTTPServer::Dispatch, this,
                                               it->h->event_loop(),
                                               std::placeholders::_1,
                                               std::placeholders::_2,
                                               callback);
            it->h->RegisterHandler(uri, cb);
        }
    }
    callbacks_[uri] = callback;
}

void HTTPServer::RegisterDefaultHandler(HTTPRequestCallback callback) {
    if (IsRunning()) {
        for (auto it = listen_threads_.begin(); it != listen_threads_.end(); ++it) {
            HTTPRequestCallback cb = std::bind(&HTTPServer::Dispatch, this,
                                               it->h->event_loop(),
                                               std::placeholders::_1,
                                               std::placeholders::_2,
                                               callback);
            it->h->RegisterDefaultHandler(cb);
        }
    }
    default_callback_ = callback;
}

void HTTPServer::Dispatch(EventLoop* listening_loop,
                          const ContextPtr& ctx,
                          const HTTPSendResponseCallback& response_callback,
                          const HTTPRequestCallback& user_callback) {
    LOG_TRACE << "dispatch request " << ctx->req << " url=" << ctx->original_uri() << " in main thread";
    EventLoop* loop = NULL;
    loop = GetNextLoop(listening_loop, ctx);


    // 将该HTTP请求调度到工作线程处理
    auto f = [](const ContextPtr & context,
                const HTTPSendResponseCallback & response_cb,
                const HTTPRequestCallback & user_cb) {
        LOG_TRACE << "process request " << context->req
            << " url=" << context->original_uri() << " in working thread";

        // 在工作线程中执行，调用上层应用设置的回调函数来处理该HTTP请求
        // 当上层应用处理完后，必须要调用 response_callback 来处理结果反馈回来，也就是会回到 Service::SendReply 函数中。
        user_cb(context, response_cb);
    };

    loop->RunInLoop(std::bind(f, ctx, response_callback, user_callback));
}


EventLoop* HTTPServer::GetNextLoop(EventLoop* default_loop, const ContextPtr& ctx) {
    if (tpool_->thread_num() == 0) {
        return default_loop;
    }

    if (IsRoundRobin()) {
        return tpool_->GetNextLoop();
    }
    uint64_t hash = std::hash<std::string>()(ctx->remote_ip);
    return tpool_->GetNextLoopWithHash(hash);
}

Service* HTTPServer::service(int index) const {
    if (index < int(listen_threads_.size())) {
        return listen_threads_[index].h.get();
    }

    return NULL;
}
}
}
