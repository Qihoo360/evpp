#include <evpp/event_loop.h>
#include <evpp/event_loop_thread_pool.h>

#include "examples/winmain-inl.h"

uint64_t clock_us() {
    return std::chrono::steady_clock::now().time_since_epoch().count() / 1000;
}

class PostTask {
public:
    PostTask(int thread_count, uint64_t post_count)
        : thread_count_(thread_count)
        , post_count_(post_count)
        , pool_(&loop_, thread_count) {}

    void Start() {
        pool_.Start(true);
        start_time_ = clock_us();

        for (int i = 0; i < thread_count_ / 2; ++i) {
            post(i * 2);
        }
    }

    void Wait() {
        loop_.Run();
    }

    double use_time() const {
        return double(stop_time_ - start_time_) / 1000000.0;
    }
private:

    void post(int thread_index) {
        pool_.GetNextLoopWithHash(thread_index)->RunInLoop(
            [this, thread_index]() mutable {
            if (count_.fetch_add(1) == post_count_) {
                stop();
                return;
            } 
            
            if (count_ <= post_count_) {
                if (thread_index % 2) {
                    thread_index -= 1;
                } else {
                    thread_index += 1;
                }

                post(thread_index);
            }
        });
    }

    void stop() {
        stop_time_ = clock_us();
        loop_.RunInLoop([this]() {
            pool_.Stop(true);
            loop_.Stop();
        });
    }
private:
    int const thread_count_;
    uint64_t const post_count_;
    evpp::EventLoop loop_;
    evpp::EventLoopThreadPool pool_;
    std::atomic<uint64_t> count_{ 0 };
    uint64_t start_time_;
    uint64_t stop_time_;
};

int main(int argc, char* argv[]) {
    int thread_count = 2;
    long long post_count = 10000;

    if (argc == 3) {
        thread_count = std::atoi(argv[1]);
        post_count = std::atoll(argv[2]);
    } else {
        printf("Usage : %s <thread-count> <post-count>\n", argv[0]);
        return 0;
    }

    PostTask p(thread_count, post_count);
    p.Start();
    p.Wait();
    LOG_WARN << argv[0] << " thread_count=" << thread_count << " post_count=" << post_count << " use time: " << p.use_time() << " seconds\n";
    return 0;
}
