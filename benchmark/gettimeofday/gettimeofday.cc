#include <chrono>
#include <functional>
#include <iostream>
#include <string>

#include <evpp/gettimeofday.h>

uint64_t gettimeofday_benchmark(int loop) {
    uint64_t rc = 0;
    for (int i = 0; i < loop; ++i) {
        auto ts = evpp::utcmicrosecond();
        rc += ts;
    }
    return rc;
}

uint64_t system_clock_benchmark(int loop) {
    uint64_t rc = 0;
    for (int i = 0; i < loop; ++i) {
        auto ts = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
        rc += ts;
    }
    return rc;
}

uint64_t high_resolution_clock_benchmark(int loop) {
    uint64_t rc = 0;
    for (int i = 0; i < loop; ++i) {
        auto ts = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
        rc += ts;
    }
    return rc;
}


uint64_t steady_clock_benchmark(int loop) {
    uint64_t rc = 0;
    for (int i = 0; i < loop; ++i) {
        auto ts = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
        rc += ts;
    }
    return rc;
}

typedef std::function<uint64_t(int)> BenchmarkFunctor;
void Benchmark(BenchmarkFunctor f, const std::string& name) {
    int loop = 1000 * 1000 * 10;
    auto start = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
    f(loop);
    auto end = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
    auto cost = double(end - start) / 1000000.0;
    std::cout << name << " loop=" << loop << " cost=" << cost << "s op=" << double(end - start) * 1000.0 / loop << "ns/op QPS=" << (loop/cost)/1024.0 << "k\n";
}


/*
Linux 3.10.0-327.28.3.el7.x86_64 test result:

         gettimeofday_benchmark loop=10000000 cost=0.3642s op= 36.427ns/op QPS=26808.5k
         system_clock_benchmark loop=10000000 cost=1.2324s op=123.249ns/op QPS=7923.51k
         steady_clock_benchmark loop=10000000 cost=1.2244s op=122.441ns/op QPS=7975.82k
high_resolution_clock_benchmark loop=10000000 cost=1.2508s op=125.082ns/op QPS=7807.36k
*/

int main() {
    Benchmark(&gettimeofday_benchmark,          "         gettimeofday_benchmark");
    Benchmark(&system_clock_benchmark,          "         system_clock_benchmark");
    Benchmark(&steady_clock_benchmark,          "         steady_clock_benchmark");
    Benchmark(&high_resolution_clock_benchmark, "high_resolution_clock_benchmark");
    return 0;
}
