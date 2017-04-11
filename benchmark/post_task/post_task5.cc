#include <evpp/event_loop.h>
#include <evpp/event_loop_thread.h>

#include "examples/winmain-inl.h"

uint64_t clock_us() {
    return std::chrono::steady_clock::now().time_since_epoch().count() / 1000;
}

typedef std::function<void()> Task;

class PostTask {
public:
    PostTask(uint64_t post_count)
        : post_count_(post_count) {
        pending_tasks_.reserve((uint32_t)post_count);
        temp_tasks_.reserve((uint32_t)post_count);
    }

    void Start() {
        loop_.Start(true);
        start_time_ = clock_us();
        for (size_t i = 0; i < post_count_; ++i) {
            post();
        }
    }

    void Wait() {
        while (!loop_.IsStopped()) {
            usleep(500000);
        }
    }

    double use_time() const {
        return double(stop_time_ - start_time_) / 1000000.0;
    }
private:
    void post() {
        bool need_post = false;

        {
            std::lock_guard<std::mutex> lock(mutex_);
            need_post = pending_tasks_.empty();
            pending_tasks_.emplace_back([this]() {
                count_ += 1;
            });
        }

        if (need_post) {
            loop_.loop()->RunInLoop([this]() {
                temp_tasks_.clear();

                {
                    std::lock_guard<std::mutex> lock(mutex_);
                    temp_tasks_.swap(pending_tasks_);
                }

                for (auto& task : temp_tasks_) {
                    task();
                }

                if (count_ == post_count_) {
                    stop_time_ = clock_us();
                    stop();
                }
            });
        }
    }
private:
    void stop() {
        stop_time_ = clock_us();
        loop_.Stop();
    }
private:
    const uint64_t post_count_;
    evpp::EventLoopThread loop_;
    uint64_t count_{ 0 };
    uint64_t start_time_;
    uint64_t stop_time_;
    std::mutex mutex_;
    std::vector<Task> pending_tasks_;
    std::vector<Task> temp_tasks_;
};

int main(int argc, char* argv[]) {
    long long post_count = 10000;

    if (argc == 2) {
        post_count = std::atoll(argv[1]);
    } else {
        printf("Usage : %s <post-count>\n", argv[0]);
        return 0;
    }

    PostTask p(post_count);
    p.Start();
    p.Wait();
    LOG_WARN << argv[0] << " post_count=" << post_count << " use time: " << p.use_time() << " seconds\n";
    return 0;
}
