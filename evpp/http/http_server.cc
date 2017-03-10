#include "http_server.h"

#include "evpp/libevent_headers.h"
#include "evpp/libevent_watcher.h"
#include "evpp/event_loop.h"
#include "evpp/event_loop_thread.h"
#include "evpp/event_loop_thread_pool.h"
#include "evpp/utility.h"

namespace evpp {
namespace http {

Server::Server(uint32_t thread_num) {
    tpool_.reset(new EventLoopThreadPool(NULL, thread_num));
}

Server::~Server() {
    tpool_.reset();
}

bool Server::Init(int listen_port) {
    ListenThread lt;
    lt.thread = std::make_shared<EventLoopThread>();
    lt.thread->SetName(std::string("StandaloneHTTPServer-Main-") + std::to_string(listen_port));

    lt.hserver = std::make_shared<Service>(lt.thread->event_loop());
    if (!lt.hserver->Listen(listen_port)) {
        int serrno = errno;
        LOG_ERROR << "http server listen at port " << listen_port << " failed. errno=" << serrno << " " << strerror(serrno);
        lt.hserver->Stop();
        return false;
    }
    listen_threads_.push_back(lt);
    return true;
}

bool Server::Init(const std::vector<int>& listen_ports) {
    bool rc = true;
    for (auto lp : listen_ports) {
        rc &= Init(lp);
    }
    return rc;
}


bool Server::Init(const std::string& listen_ports/*"80,8080,443"*/) {
    std::vector<std::string> vec;
    StringSplit(listen_ports, ",", 0, vec);

    std::vector<int> v;
    for (auto& s : vec) {
        int i = std::atoi(s.c_str());
        if (i <= 0) {
            LOG_ERROR << "Cannot convert [" << s << "] to a integer. 'listen_ports' format wrong.";
            return false;
        }
        v.push_back(i);
    }

    return Init(v);
}

bool Server::AfterFork() {
    for (auto& lt : listen_threads_) {
        lt.thread->event_loop()->AfterFork();
    }
    return true;
}

bool Server::Start() {
    bool rc = tpool_->Start(true);
    for (auto& lt : listen_threads_) {
        auto http_close_fn = std::bind(&Service::Stop, lt.hserver);
        rc &= lt.thread->Start(true,
                               EventLoopThread::Functor(),
                               http_close_fn);
        assert(lt.thread->IsRunning());
        for (auto& c : callbacks_) {
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
        if (!rc) {
            return false;
        }
    }
    while (!IsRunning()) {
        usleep(1);
    }
    LOG_TRACE << "http server is running" ;
    return true;
}

void Server::Stop(bool wait_thread_exit /*= false*/) {
    for (auto& lt : listen_threads_) {
        // 1. Service::Stop will be called automatically when listen_thread_ is existing
        // 2. EventLoopThread::Stop will be called to terminate the thread
        lt.thread->Stop();
    }
    tpool_->Stop();

    if (!wait_thread_exit) {
        return;
    }

    for (;;) {
        bool stopped = true;
        for (auto& lt : listen_threads_) {
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
    for (auto& lt : listen_threads_) {
        (void)lt; // avoid compile warning
        assert(lt.thread->IsStopped());
    }
    assert(IsStopped());
}

void Server::Pause() {
    for (auto& lt : listen_threads_) {
        EventLoop* loop = lt.thread->event_loop();
        auto f = [&lt]() {
            lt.hserver->Pause();
        };
        loop->RunInLoop(f);
    }
}

void Server::Continue() {
    for (auto& lt : listen_threads_) {
        EventLoop* loop = lt.thread->event_loop();
        auto f = [&lt]() {
            lt.hserver->Continue();
        };
        loop->RunInLoop(f);
    }
}

bool Server::IsRunning() const {
    if (listen_threads_.empty()) {
        return false;
    }

    if (!tpool_->IsRunning()) {
        return false;
    }

    for (auto& lt : listen_threads_) {
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

    for (auto& lt : listen_threads_) {
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
    // Make sure it is running in the HTTP listening thread
    assert(listening_loop->IsInLoopThread());
    LOG_TRACE << "dispatch request " << ctx->req() << " url=" << ctx->original_uri() << " in main thread";
    EventLoop* loop = NULL;
    loop = GetNextLoop(listening_loop, ctx);

    // Forward this HTTP request to a worker thread to process
    auto f = [loop, ctx, response_callback, user_callback]() {
        LOG_TRACE << "process request " << ctx->req()
                  << " url=" << ctx->original_uri() << " in working thread";

        // This is in the worker thread.
        // Invoke user layer handler to process this HTTP process.
        // After the user layer finished processing,
        // the user layer has responsibility to invoke response_cb
        // to send the result back to framework,
        // that actually comes back to Service::SendReply method.
        assert(loop->IsInLoopThread());
        user_callback(loop, ctx, response_callback);
    };

    loop->RunInLoop(f);
}


EventLoop* Server::GetNextLoop(EventLoop* default_loop, const ContextPtr& ctx) {
    if (tpool_->thread_num() == 0) {
        return default_loop;
    }

    if (IsRoundRobin()) {
        return tpool_->GetNextLoop();
    }

#if LIBEVENT_VERSION_NUMBER >= 0x02010500
    const sockaddr*  sa = evhttp_connection_get_addr(ctx->req()->evcon);
    if (sa) {
        const sockaddr_in* r = sock::sockaddr_in_cast(sa);
        LOG_INFO << "http remote address " << sock::ToIPPort(r);
        return tpool_->GetNextLoopWithHash(r->sin_addr.s_addr);
    } else {
        uint64_t hash = std::hash<std::string>()(ctx->remote_ip());
        return tpool_->GetNextLoopWithHash(hash);
    }
#else
    uint64_t hash = std::hash<std::string>()(ctx->remote_ip());
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
