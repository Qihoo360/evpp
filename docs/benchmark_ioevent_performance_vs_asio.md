The IO Event performance benchmark test of [evpp] against [Boost.Asio]



### Brief

We do a throughput benchmark test here [benchmark_throughput_vs_asio](benchmark_throughput_vs_asio.md) which shows [evpp] has a similar performance with [asio].

And we also do another benchmark about IO event performance and throughput here [benchmark_ping_pong_spend_time_vs_asio.md](benchmark_ping_pong_spend_time_vs_asio.md) which shows [evpp]'s performance is about **5%~20%** higher than [asio].

Now, we do the third benchmark which is only for benchmarking of IO event performance. We take the benchmark method from [http://libev.schmorp.de/bench.html](http://libev.schmorp.de/bench.html). 

The benchmark is very simple: first a number of socket pairs is created, then event watchers for those pairs are installed and then a (smaller) number of "active clients" send and receive data on a subset of those sockets.


### Environment

1. Linux CentOS 6.2, 2.6.32-220.7.1.el6.x86_64
2. Intel(R) Xeon(R) CPU E5-2630 v2 @ 2.60GHz
3. gcc version 4.8.2 20140120 (Red Hat 4.8.2-15) (GCC) 


### The testing object

1. [evpp-v0.2.4](https://github.com/Qihoo360/evpp/archive/v0.2.4.zip) based on libevent-2.0.21
2. [asio-1.10.8](http://www.boost.org/)
3. [libevent_ioevent_bench.c](https://github.com/Qihoo360/evpp/blob/master/benchmark/ioevent/libevent/libevent_ioevent_bench.c) is taken from [libevent] based on libevent-2.0.21

The test code of [evpp] is at the source code `benchmark/ioevent/evpp`. We use `evpp::FdChannel` to implement this test program [evpp_FdChannel](https://github.com/Qihoo360/evpp/blob/master/benchmark/ioevent/evpp/evpp_ioevent_bench.cc) and use `evpp::PipeEventWatcher` to implement another test program [evpp_PipeEventWatcher](https://github.com/Qihoo360/evpp/blob/master/benchmark/ioevent/evpp/evpp_ioevent_pipe_watcher.cc). We use `tools/benchmark-build.sh` to compile it. The test script is [run_ioevent_bench.sh](https://github.com/Qihoo360/evpp/blob/master/benchmark/ioevent/run_ioevent_bench.sh), showing as below:

```shell
for num in 500 1000 10000 30000; do
for loop in 1 2 3 4 5 6 7 8 9 10; do
    taskset -c 3 ../../build-release/bin/benchmark_ioevent_evpp_pipe_watcher -n $num -a 100 -w $num 
    taskset -c 3 ../../build-release/bin/benchmark_ioevent_evpp -n $num -a 100 -w $num 
    taskset -c 3 ../../build-release/bin/benchmark_ioevent_libevent -n $num -a 100 -w $num 
done
done
```

The test code of [asio] is at [https://github.com/huyuguang/asio_benchmark](https://github.com/huyuguang/asio_benchmark) using commits `21fc1357d59644400e72a164627c1be5327fbe3d` and the test program [socketpair.cpp](https://github.com/huyuguang/asio_benchmark/blob/master/socketpair.cpp). The test script is [run_ioevent_bench.sh](https://github.com/Qihoo360/evpp/blob/master/benchmark/ioevent/asio/run_ioevent_bench.sh). It is：

```shell
for num in 500 1000 10000 30000; do
for loop in 1 2 3 4 5 6 7 8 9 10; do
    echo "Bench index=$loop num=$num" 
    taskset -c 3 ./asio_test.exe socketpair $num 100 $num
done
done
```

### Benchmark conclusion

1. In all the cases, [evpp]'s two implementation program [evpp_FdChannel](https://github.com/Qihoo360/evpp/blob/master/benchmark/ioevent/evpp/evpp_ioevent_bench.cc) and [evpp_PipeEventWatcher](https://github.com/Qihoo360/evpp/blob/master/benchmark/ioevent/evpp/evpp_ioevent_pipe_watcher.cc) both have a higher performance than [asio], about **20%~50%**.
2. The native [libevent]'s benchmark program [libevent_ioevent_bench.c](https://github.com/Qihoo360/evpp/blob/master/benchmark/ioevent/libevent/libevent_ioevent_bench.c) have the highest performance. It have a little bit advantages against [evpp], which is about 2%, that is acceptable, we think.

For details, please see the charts below. The horizontal axis is the number of pipe count. The vertical axis is the time spent, the lower is the better.

![](https://raw.githubusercontent.com/zieckey/resources/master/evpp/benchmark/ioevent/evpp-vs-asio-ioevent-benchmark.png)

### All benchmark reports

[The IO Event performance benchmark against Boost.Asio](benchmark_ioevent_performance_vs_asio.md) : [evpp] is higher than [asio] about **20%~50%** in this case

[The ping-pong benchmark against Boost.Asio](benchmark_ping_pong_spend_time_vs_asio.md) : [evpp] is higher than [asio] about **5%~20%** in this case

[The throughput benchmark against libevent2](benchmark_throughput_vs_libevent.md) : [evpp] is higher than [libevent] about **17%~130%** in this case 

[The performance benchmark of `queue with std::mutex` against `boost::lockfree::queue` and `moodycamel::ConcurrentQueue`](benchmark_lockfree_vs_mutex.md) : `moodycamel::ConcurrentQueue` is the best, the average is higher than `boost::lockfree::queue` about **25%~100%** and higher than `queue with std::mutex` about **100%~500%**

[The throughput benchmark against Boost.Asio](benchmark_throughput_vs_asio.md) : [evpp] and [asio] have the similar performance in this case

[The throughput benchmark against Boost.Asio(中文)](benchmark_throughput_vs_asio_cn.md) : [evpp] and [asio] have the similar performance in this case

[The throughput benchmark against muduo(中文)](benchmark_throughput_vs_muduo_cn.md) : [evpp] and [muduo] have the similar performance in this case

### Last

The beautiful chart is rendered by [gochart]. Thanks for your reading this report. Please feel free to discuss with us for the benchmark test.

[Boost.Asio]:http://www.boost.org/
[boost.asio]:http://www.boost.org/
[asio]:http://www.boost.org/
[boost]:http://www.boost.org/
[evpp]:https://github.com/Qihoo360/evpp
[muduo]:https://github.com/chenshuo/muduo
[libevent2]:https://github.com/libevent/libevent
[libevent]:https://github.com/libevent/libevent
[Golang]:https://golang.org
[Buffer]:https://github.com/Qihoo360/evpp/blob/master/evpp/buffer.h
[recipes]:https://github.com/chenshuo/recipes
[gochart]:https://github.com/zieckey/gochart/