The throughput benchmark test of [evpp] against [libevent]

### Brief

[evpp] is based on [libevent]. So we do a benchmark test against [libevent] is meaningful. 

### The testing object

1. [evpp-v0.2.4](https://github.com/Qihoo360/evpp/archive/v0.2.4.zip) based on libevent-2.0.21
3. [libevent/server.c](https://github.com/Qihoo360/evpp/blob/master/benchmark/throughput/libevent/server.c) and [libevent/client.c](https://github.com/Qihoo360/evpp/blob/master/benchmark/throughput/libevent/client.c) are taken from [libevent] based on libevent-2.0.21

### Environment

1. Linux CentOS 6.2, 2.6.32-220.7.1.el6.x86_64
2. Intel(R) Xeon(R) CPU E5-2630 v2 @ 2.60GHz
3. gcc version 4.8.2 20140120 (Red Hat 4.8.2-15) (GCC) 


### Test method

We use the test method described at [http://think-async.com/Asio/LinuxPerformanceImprovements](http://think-async.com/Asio/LinuxPerformanceImprovements) using ping-pong protocol to do the throughput benchmark.

Simply to explains that the ping pong protocol is the client and the server both implements the echo protocol. When the TCP connection is established, the client sends some data to the server, the server echoes the data, and then the client echoes to the server again and again. The data will be the same as the table tennis in the client and the server back and forth between the transfer until one side disconnects. This is a common way to test throughput.
 
The test code of [evpp] is at the source code `benchmark/throughput/evpp`, and at here [https://github.com/Qihoo360/evpp/tree/master/benchmark/throughput/evpp](https://github.com/Qihoo360/evpp/tree/master/benchmark/throughput/evpp). We use `tools/benchmark-build.sh` to compile it. The test script is [single_thread.sh](https://github.com/Qihoo360/evpp/blob/master/benchmark/throughput/evpp/single_thread.sh). 

The test code of [libevent] is at the source code `benchmark/throughput/libevent`, and at here [libevent/server.c](https://github.com/Qihoo360/evpp/blob/master/benchmark/throughput/libevent/server.c) and [libevent/client.c](https://github.com/Qihoo360/evpp/blob/master/benchmark/throughput/libevent/client.c). The test script is [single_thread.sh](https://github.com/Qihoo360/evpp/blob/master/benchmark/throughput/libevent/single_thread.sh)

### Benchmark conclusion

1. When message is less than 4096, the throughput benchmark of [evpp] is about **17%** higher than [libevent2].
2. When the message is larger than 4096, [evpp] is much faster than [libevent], about **40%~130%** higher than [libevent2]

Although [evpp] is based on [libevent], [evpp] has a better throughput benchmark than [libevent]. There are two reasons:

1. [evpp] implements its own IO buffer instead of [libevent]'s evbuffer. 
2. [libevent] will read 4096 bytes at most every time, that is the key point why [libevent] is much slower than [evpp].   

For details, see the chart below, the horizontal axis is the number of concurrent connections. The vertical axis is the throughput, the bigger the better.

![](https://raw.githubusercontent.com/zieckey/resources/master/evpp/benchmark/throughput/evpp-vs-libevent-1thread-1024.png)
![](https://raw.githubusercontent.com/zieckey/resources/master/evpp/benchmark/throughput/evpp-vs-libevent-1thread-2048.png)
![](https://raw.githubusercontent.com/zieckey/resources/master/evpp/benchmark/throughput/evpp-vs-libevent-1thread-4096.png)
![](https://raw.githubusercontent.com/zieckey/resources/master/evpp/benchmark/throughput/evpp-vs-libevent-1thread-8192.png)
![](https://raw.githubusercontent.com/zieckey/resources/master/evpp/benchmark/throughput/evpp-vs-libevent-1thread-16384.png)

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