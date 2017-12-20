#include "http_server.h"


#include "evpp/libevent.h"
#include "evpp/event_watcher.h"
#include "evpp/event_loop.h"
#include "evpp/event_loop_thread.h"
#include "evpp/event_loop_thread_pool.h"
#include "evpp/utility.h"

#include <future>

namespace evpp {
namespace http {

Server::Server(uint32_t thread_num) {
    DLOG_TRACE;
    tpool_.reset(new EventLoopThreadPool(nullptr, thread_num));
}

Server::~Server() {
    DLOG_TRACE;
    if (!listen_threads_.empty()) {
        for (auto& lt : listen_threads_) {
            lt.thread->Join();
        }
        listen_threads_.clear();
    }

    if (tpool_) {
        assert(tpool_->IsStopped());
        tpool_->Join();
        tpool_.reset();
    }
}

bool Server::Init(int listen_port) {
    status_.store(kInitializing);
    ListenThread lt;
    lt.thread = std::make_shared<EventLoopThread>();
    lt.thread->set_name(std::string("StandaloneHTTPServer-Main-") + std::to_string(listen_port));

    lt.hservice = std::make_shared<Service>(lt.thread->loop());
    if (!lt.hservice->Listen(listen_port)) {
        int serrno = errno;
        LOG_ERROR << "this=" << this << " http server listen at port " << listen_port << " failed. errno=" << serrno << " " << strerror(serrno);
        lt.hservice->Stop();
        return false;
    }
    listen_threads_.push_back(lt);
    status_.store(kInitialized);
    return true;
}

bool Server::Init(const std::vector<int>& listen_ports) {
    status_.store(kInitializing);
    bool rc = true;
    for (auto lp : listen_ports) {
        rc = rc && Init(lp);
    }
    if (rc) {
        status_.store(kInitialized);
    } else {
        status_.store(kInitializing);
    }
    return rc;
}

bool Server::Init(const std::string& listen_ports/*"80,8080,443"*/) {
    status_.store(kInitializing);
    std::vector<std::string> vec;
    StringSplit(listen_ports, ",", 0, vec);

    std::vector<int> v;
    for (auto& s : vec) {
        int i = std::atoi(s.c_str());
        if (i <= 0) {
            LOG_ERROR << "this=" << this << " Cannot convert [" << s << "] to a integer. 'listen_ports' format wrong.";
            return false;
        }
        v.push_back(i);
    }

    auto rc = Init(v);
    if (rc) {
        status_.store(kInitialized);
    } else {
        status_.store(kInitializing);
    }
    return rc;
}

void Server::AfterFork() {
    for (auto& lt : listen_threads_) {
        lt.thread->loop()->AfterFork();
    }
    tpool_->AfterFork();
}

bool Server::Start() {
    assert(status_.load() == kInitialized);
    status_.store(kStarting);
    bool rc = tpool_->Start(true);
    if (!rc) {
        LOG_ERROR << "this=" << this << " start thread pool failed.";
        return false;
    }

    for (auto& lt : listen_threads_) {
        auto& hservice = lt.hservice;
        auto& lthread = lt.thread;
        auto http_close_fn = [hservice, this]() {
            hservice->Stop();
            DLOG_TRACE << "http service at 0.0.0.0:" << hservice->port() << " has stopped.";
            return EventLoopThread::kOK;
        };
        rc = lthread->Start(true,
                            EventLoopThread::Functor(),
                            http_close_fn);
        if (!rc) {
            LOG_ERROR << "this=" << this << " start listening thread failed.";
            return false;
        }

        using namespace std::placeholders;
        assert(lthread->IsRunning());
        for (auto& c : callbacks_) {
            auto cb = std::bind(&Server::Dispatch, this, _1, _2, _3, c.second);
            hservice->RegisterHandler(c.first, cb);
        }

        if (default_callback_) {
            auto cb = std::bind(&Server::Dispatch, this, _1, _2, _3, default_callback_);
            hservice->RegisterDefaultHandler(cb);
        }
    }

    assert(rc);


    auto is_running = [this]() {
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
    };

    while (!is_running()) {
        usleep(1);
    }
    DLOG_TRACE << "http server is running" ;
    status_.store(kRunning);
    return true;
}

void Server::Stop() {
    assert(IsRunning());
    DLOG_TRACE << "http server is stopping";

    status_.store(kStopping);

    std::promise<void> promise;
    std::atomic<int> count(0);

    // Firstly we pause all the listening threads to accept new requests.
    substatus_.store(kStoppingListener);
    for (auto& lt : listen_threads_) {
        std::shared_ptr<Service>& hs = lt.hservice;
        auto fn = [&count, &promise, this, hs]() {
            hs->Pause();
            if (count.fetch_add(1) + 1 == static_cast<int>(listen_threads_.size())) {
                promise.set_value();
            }
        };
        lt.thread->loop()->RunInLoop(fn);
    }
    promise.get_future().wait();

    // Secondly we stop thread pool
    substatus_.store(kStoppingThreadPool);
    tpool_->Stop(true);

    // Thirdly we stop the listening threads
    for (auto& lt : listen_threads_) {
        // Service::Stop will be called automatically when listen_thread_ is existing
        lt.thread->Stop(true);
    }

    status_.store(kStopped);

    // Lastly
    // Make sure all the working threads totally stopped.
    tpool_->Join();
    tpool_.reset();

    // Make sure all the listening threads totally stopped.
    for (auto& lt : listen_threads_) {
        lt.thread->Join();
    }
    listen_threads_.clear();

    DLOG_TRACE << "http server stopped";
}

void Server::Pause() {
    DLOG_TRACE << "http server pause";
    for (auto& lt : listen_threads_) {
        EventLoop* loop = lt.thread->loop();
        std::shared_ptr<Service>& hs = lt.hservice;
        auto f = [hs]() {
            hs->Pause();
        };
        loop->RunInLoop(f);
    }
}

void Server::Continue() {
    DLOG_TRACE << "http server continue";
    for (auto& lt : listen_threads_) {
        EventLoop* loop = lt.thread->loop();
        std::shared_ptr<Service>& hs = lt.hservice;
        auto f = [hs]() {
            hs->Continue();
        };
        loop->RunInLoop(f);
    }
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
    DLOG_TRACE << "dispatch request " << ctx->req() << " url=" << ctx->original_uri() << " in main thread. status=" << StatusToString();
    if (!IsRunning()) {
        LOG_WARN << "The listening thread is not running, may be it is stopping now.";
        //TODO gracefully shutdown.
        return;
    }

    EventLoop* loop = nullptr;
    loop = GetNextLoop(listening_loop, ctx);

    // Forward this HTTP request to a worker thread to process
    auto f = [loop, ctx, response_callback, user_callback, this]() {
        DLOG_TRACE << "process request " << ctx->req()
            << " url=" << ctx->original_uri()
            << " in working thread. status=" << StatusToString();
        
        if (!IsRunning()) {
            LOG_WARN << "The listening thread is not running, may be it is stopping now.";
            //TODO gracefully shutdown.
            return;
        }

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
        return listen_threads_[index].hservice.get();
    }

    return nullptr;
}
}
}
