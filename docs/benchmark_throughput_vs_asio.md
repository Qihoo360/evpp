The throughput benchmark test of [evpp] against [Boost.Asio]

[中文版：evpp与Boost.Asio吞吐量对比](benchmark_throughput_vs_asio_cn.md)

### Brief

[Boost.Asio] is a cross-platform C++ library for network and low-level I/O programming that provides developers with a consistent asynchronous model using a modern C++ approach.

### The testing object

1. [evpp-v0.2.4](https://github.com/Qihoo360/evpp/archive/v0.2.4.zip) based on libevent-2.0.21
2. [asio-1.10.8](http://www.boost.org/)

### Environment

1. Linux CentOS 6.2, 2.6.32-220.7.1.el6.x86_64
2. Intel(R) Xeon(R) CPU E5-2630 v2 @ 2.60GHz
3. gcc version 4.8.2 20140120 (Red Hat 4.8.2-15) (GCC) 


### Test method

We use the test method described at [http://think-async.com/Asio/LinuxPerformanceImprovements](http://think-async.com/Asio/LinuxPerformanceImprovements) using ping-pong protocol to do the throughput benchmark.

Simply to explains that the ping pong protocol is the client and the server both implements the echo protocol. When the TCP connection is established, the client sends some data to the server, the server echoes the data, and then the client echoes to the server again and again. The data will be the same as the table tennis in the client and the server back and forth between the transfer until one side disconnects. This is a common way to test throughput.
 
The test code of [evpp] is at the source code `benchmark/throughput/evpp`, and at here [https://github.com/Qihoo360/evpp/tree/master/benchmark/throughput/evpp](https://github.com/Qihoo360/evpp/tree/master/benchmark/throughput/evpp). We use `tools/benchmark-build.sh` to compile it. The test script are [single_thread.sh](https://github.com/Qihoo360/evpp/blob/master/benchmark/throughput/evpp/single_thread.sh) and [multiple_thread.sh](https://github.com/Qihoo360/evpp/blob/master/benchmark/throughput/evpp/multiple_thread.sh). 

The test code of [asio] is at [https://github.com/huyuguang/asio_benchmark](https://github.com/huyuguang/asio_benchmark) using commits `21fc1357d59644400e72a164627c1be5327fbe3d` and the `client2.cpp/server2.cpp` test code. The test script is [single_thread.sh](https://github.com/Qihoo360/evpp/blob/master/benchmark/throughput/asio/single_thread.sh) and [multiple_thread.sh](https://github.com/Qihoo360/evpp/blob/master/benchmark/throughput/asio/multiple_thread.sh). 

We have done two benchmarks:

1. Single thread : When the number of concurrent connections is 1/10/100/1000/10000, the message size is 1024/2048/4096/8192/16384/81920.
2. Multi-threaded : When the number of concurrent connections is 100 or 1000, the number of threads in the server and the client is set to 2/3/4/6/8, the message size is 16384 bytes


### Benchmark conclusion

#### Single Thread

1. When the number of concurrent connections is 10,000 or more in the test, [asio] is better, the average is higher than [evpp] **5%~10%**
2. When the number of concurrent connections is 1,10,100,1000 in the test, [evpp]'s performance is better, the average is higher than [asio] **10%~20%**

For details, see the chart below, the horizontal axis is the number of concurrent connections. The vertical axis is the throughput, the bigger the better.

![](https://raw.githubusercontent.com/zieckey/resources/master/evpp/benchmark/throughput/evpp-vs-asio-1thread-1024.png)
![](https://raw.githubusercontent.com/zieckey/resources/master/evpp/benchmark/throughput/evpp-vs-asio-1thread-2048.png)
![](https://raw.githubusercontent.com/zieckey/resources/master/evpp/benchmark/throughput/evpp-vs-asio-1thread-4096.png)
![](https://raw.githubusercontent.com/zieckey/resources/master/evpp/benchmark/throughput/evpp-vs-asio-1thread-8192.png)
![](https://raw.githubusercontent.com/zieckey/resources/master/evpp/benchmark/throughput/evpp-vs-asio-1thread-16384.png)
![](https://raw.githubusercontent.com/zieckey/resources/master/evpp/benchmark/throughput/evpp-vs-asio-1thread-81920.png)

#### Multi thread

1. When the number of concurrent connections is 1000, [evpp] and [asio] have a similar performance and have their own advantages.
2. When the number of concurrent connections is 100, [asio] is better performing to **10%** in this case.

For details, see the chart below. The horizontal axis is the number of threads. The vertical axis is the throughput, the bigger the better.

![](https://raw.githubusercontent.com/zieckey/resources/master/evpp/benchmark/throughput/evpp-vs-asio-multi-thread-100connection-16384.png)
![](https://raw.githubusercontent.com/zieckey/resources/master/evpp/benchmark/throughput/evpp-vs-asio-multi-thread-1000connection-16384.png)

### Analysis

In this ping pong benchmark test, the [asio]'s test code is using a fixed-size buffer to receive and send the data. That can  take advantage of [asio] `Proactor` model, he has almost no memory allocation. Each time [asio] only reads fixed size of data and then sent it out, and then use the same BUFFER to do the next read operation. 

In the same time [evpp] is a network library of `Reactor` model, the receiving data is probably not a fixed size, which involves `evpp::Buffer` internal memory reallocation problems that lead to excessive memory allocation.

We will do another benchmark test to verify the analysis. Please look forward to it.

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