#include "evpp/httpc/conn_pool.h"
#include "evpp/httpc/conn.h"

namespace evpp {
namespace httpc {
ConnPool::ConnPool(const std::string& h, int p, Duration t, size_t size)
    : host_(h), port_(p), timeout_(t), max_pool_size_(size) {
}

ConnPool::~ConnPool() {
    assert(pool_.empty());
}

ConnPtr ConnPool::Get(EventLoop* loop) {
    assert(loop->IsInLoopThread());
    auto it = pool_.find(loop);
    if (it == pool_.end()) {
        std::lock_guard<std::mutex> guard(mutex_);
        pool_[loop] = std::vector<ConnPtr>();
    }

    it = pool_.find(loop);
    assert(it != pool_.end());

    ConnPtr c;
    if (it->second.empty()) {
        c.reset(new Conn(this, loop));
        return c;
    }

    c = it->second.back();
    it->second.pop_back();
    return c;
}

void ConnPool::Put(const ConnPtr& c) {
    EventLoop* loop = c->loop();
    assert(loop->IsInLoopThread());
    auto it = pool_.find(loop);
    assert(it != pool_.end());
    if (it->second.size() >= max_pool_size_) {
        return;
    }
    it->second.push_back(c);
}

void ConnPool::Clear() {
    if (pool_.empty()) {
        return;
    }

    std::map<EventLoop*, std::vector<ConnPtr> > map;
    if (!pool_.empty()) {
        std::lock_guard<std::mutex> guard(mutex_);
        pool_.swap(map);
        assert(pool_.empty());
    }

    // Make sure delete Conn in its own EventLoop thread
    for (auto& m : map) {
        for (auto& c : m.second) {
            m.first->RunInLoop(std::bind(&Conn::Close, c));
        }
        m.second.clear();
    }

    pool_.clear();
}
}
}