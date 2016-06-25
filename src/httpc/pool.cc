#include "evpp/httpc/pool.h"
#include "evpp/httpc/conn.h"

namespace evpp {
    namespace httpc {
        Pool::Pool(const std::string& h, int p, Duration t, size_t size)
            : host_(h), port_(p), timeout_(t), max_pool_size_(size) {
        }

        HTTPConnPtr Pool::Get(EventLoop* loop) {
            auto it = pools_.find(loop);
            if (it == pools_.end()) {
                std::lock_guard<std::mutex> guard(mutex_);
                pools_[loop] = std::vector<HTTPConnPtr>();
            }

            it = pools_.find(loop);
            assert(it != pools_.end());

            HTTPConnPtr c;
            if (it->second.empty()) {
                c.reset(new Conn(this, loop));
                return c;
            }
            c = it->second.back();
            it->second.pop_back();
            return c;
        }

        void Pool::Put(const HTTPConnPtr& c) {
            EventLoop* loop = c->loop();
            loop->AssertInLoopThread();
            auto it = pools_.find(loop);
            assert(it != pools_.end());
            if (it->second.size() >= max_pool_size_) {
                return;
            }
            it->second.push_back(c);
        }

    } // httpc
} // evpp