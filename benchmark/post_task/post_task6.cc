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
            , total_post_count_(post_count)
            , pool_(NULL, thread_count) {
        }

        void Start() {
            pool_.Start(true);
            loop_.Start(true);
            start_time_ = clock_us();
            post();
        }

        void Wait() {
            while (!loop_.IsStopped() && !pool_.IsStopped()) {
                usleep(1000);
            }
        }

        double use_time() const {
            return double(stop_time_ - start_time_)/1000000.0;
        }

private:
    void post() {
        auto p = [this]() {
            for (uint64_t i = 0; i < total_post_count_; i++) {
                loop_.loop()->RunInLoop([this]() {
                    count_++;
                    if (count_ == total_post_count_ * pool_.thread_num()) {
                        Stop();
                    }
                });
            }
        };


        for (uint32_t i = 0; i < pool_.thread_num(); i++) {
            pool_.GetNextLoopWithHash(i)->RunInLoop(p);
        }
        
    }

    void Stop() {
        stop_time_ = clock_us();
        pool_.Stop();
        loop_.Stop();
    }
private:
    const int thread_count_;
    const uint64_t total_post_count_;
    uint64_t count_ = 0;
    evpp::EventLoopThread loop_;
    evpp::EventLoopThreadPool pool_;
    uint64_t start_time_ = 0;
    uint64_t stop_time_ = 0;
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
