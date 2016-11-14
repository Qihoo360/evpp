#include "http_server.h"

#include "evpp/libevent_headers.h"
#include "evpp/libevent_watcher.h"
#include "evpp/event_loop.h"
#include "evpp/event_loop_thread.h"
#include "evpp/event_loop_thread_pool.h"

namespace evpp {
namespace http {

Server::Server(uint32_t thread_num) {
    tpool_.reset(new EventLoopThreadPool(NULL, thread_num));
}

Server::~Server() {
    tpool_.reset();
}

bool Server::Start(int port) {
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

bool Server::Start(std::vector<int> listen_ports) {
    bool rc = tpool_->Start(true);
    if (!rc) {
        return rc;
    }

    for (size_t i = 0; i < listen_ports.size(); ++i) {
        if (!StartListenThread(listen_ports[i])) {
            return false;
        }
    }

    while (!IsRunning()) {
        usleep(1);
    }

    return true;
}

bool Server::StartListenThread(int port) {
    ListenThread lt;
    lt.thread = std::make_shared<EventLoopThread>();
    lt.thread->SetName(std::string("StandaloneHTTPServer-Main-") + std::to_string(port));

    lt.hserver = std::make_shared<Service>(lt.thread->event_loop());
    if (!lt.hserver->Listen(port)) {
        int serrno = errno;
        LOG_ERROR << "http server listen at port " << port << " failed. errno=" << serrno << " " << strerror(serrno);
        lt.hserver->Stop();
        return false;
    }

    // 当 ListenThread 退出时，会调用该函数来停止 Service
    auto http_close_fn = std::bind(&Service::Stop, lt.hserver);
    bool rc = lt.thread->Start(true,
                          EventLoopThread::Functor(),
                          http_close_fn);
    assert(lt.thread->IsRunning());
    for (auto &c : callbacks_) {
        auto cb = std::bind(&Server::Dispatch, this,
                                           std::placeholders::_1,
                                           std::placeholders::_2,
                                           std::placeholders::_3,
                                           c.second);
        lt.hserver->RegisterHandler(c.first, cb);
    }
    HTTPRequestCallback cb = std::bind(&Server::Dispatch, this,
                                       std::placeholders::_1,
                                       std::placeholders::_2,
                                       std::placeholders::_3,
                                       default_callback_);
    lt.hserver->RegisterDefaultHandler(cb);
    listen_threads_.push_back(lt);
    LOG_TRACE << "http server is running at " << port;
    return rc;
}

void Server::Stop(bool wait_thread_exit /*= false*/) {
    for (auto &lt : listen_threads_) {
        // 1. Service 对象的Stop会在 listen_thread_ 退出时自动执行 Service::Stop
        // 2. EventLoopThread 对象必须调用 Stop 来停止
        lt.thread->Stop();
    }
    tpool_->Stop();

    if (!wait_thread_exit) {
        return;
    }

    for (;;) {
        bool stopped = true;
        for (auto &lt : listen_threads_) {
            if (!lt.thread->IsStopped()) {
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

    assert(tpool_->IsStopped());
    for (auto &lt : listen_threads_) {
        assert(lt.thread->IsStopped());
    }
    assert(IsStopped());
}

void Server::Pause() {
    for (auto &lt : listen_threads_) {
        EventLoop* loop = lt.thread->event_loop();
        loop->RunInLoop(std::bind(&Service::Pause, lt.hserver));
    }
}

void Server::Continue() {
    for (auto &lt : listen_threads_) {
        EventLoop* loop = lt.thread->event_loop();
        loop->RunInLoop(std::bind(&Service::Continue, lt.hserver));
    }
}

bool Server::IsRunning() const {
    if (listen_threads_.empty()) {
        return false;
    }

    if (!tpool_->IsRunning()) {
        return false;
    }

    for (auto &lt : listen_threads_) {
        if (!lt.thread->IsRunning()) {
            return false;
        }
    }
    return true;
}

bool Server::IsStopped() const {
    assert(!listen_threads_.empty());
    assert(tpool_);

    if (!tpool_->IsStopped()) {
        return false;
    }

    for (auto &lt : listen_threads_) {
        if (!lt.thread->IsStopped()) {
            return false;
        }
    }
    return true;
}

void Server::RegisterHandler(const std::string& uri, HTTPRequestCallback callback) {
    assert(!IsRunning());
    callbacks_[uri] = callback;
}

void Server::RegisterDefaultHandler(HTTPRequestCallback callback) {
    assert(!IsRunning());
    default_callback_ = callback;
}

void Server::Dispatch(EventLoop* listening_loop,
                      const ContextPtr& ctx,
                      const HTTPSendResponseCallback& response_callback,
                      const HTTPRequestCallback& user_callback) {
    // 当前正在 HTTP 的监听线程中执行
    assert(listening_loop->IsInLoopThread());
    LOG_TRACE << "dispatch request " << ctx->req << " url=" << ctx->original_uri() << " in main thread";
    EventLoop* loop = NULL;
    loop = GetNextLoop(listening_loop, ctx);

    // 将该HTTP请求调度到工作线程处理
    auto f = [](EventLoop* l, const ContextPtr & context,
                const HTTPSendResponseCallback & response_cb,
                const HTTPRequestCallback & user_cb) {
        LOG_TRACE << "process request " << context->req
            << " url=" << context->original_uri() << " in working thread";

        // 在工作线程中执行，调用上层应用设置的回调函数来处理该HTTP请求
        // 当上层应用处理完后，上层应用必须调用 response_cb 将处理结果反馈回来，也就是会回到 Service::SendReply 函数中。
        user_cb(l, context, response_cb);
    };

    loop->RunInLoop(std::bind(f, loop, ctx, response_callback, user_callback));
}


EventLoop* Server::GetNextLoop(EventLoop* default_loop, const ContextPtr& ctx) {
    if (tpool_->thread_num() == 0) {
        return default_loop;
    }

    if (IsRoundRobin()) {
        return tpool_->GetNextLoop();
    }

#if LIBEVENT_VERSION_NUMBER >= 0x02010500
    const sockaddr*  sa = evhttp_connection_get_addr(ctx->req->evcon);
    if (sa) {
        const sockaddr_in* r = sock::sockaddr_in_cast(sa);
        LOG_INFO << "http remote address " << sock::ToIPPort(r);
        return tpool_->GetNextLoopWithHash(r->sin_addr.s_addr);
    } else {
        uint64_t hash = std::hash<std::string>()(ctx->remote_ip);
        return tpool_->GetNextLoopWithHash(hash);
    }
#else
    uint64_t hash = std::hash<std::string>()(ctx->remote_ip);
    return tpool_->GetNextLoopWithHash(hash);
#endif
}

Service* Server::service(int index) const {
    if (index < int(listen_threads_.size())) {
        return listen_threads_[index].hserver.get();
    }

    return NULL;
}
}
}
