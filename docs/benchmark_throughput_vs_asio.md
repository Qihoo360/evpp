The throughput benchmark test : [evpp] VS [Boost.Asio]

[中文版：evpp与Boost.Asio吞吐量对比](benchmark_throughput_vs_asio_cn.md)

### Brief

[Boost.Asio] is a cross-platform C++ library for network and low-level I/O programming that provides developers with a consistent asynchronous model using a modern C++ approach.

### The testing object

1. [evpp-v0.2.4](https://github.com/Qihoo360/evpp/archive/v0.2.4.zip) based on libevent-2.0.21
2. [asio-1.10.8](https://github.com/chenshuo/muduo/archive/v1.0.9.zip)

### Environment

1. Linux CentOS 6.2, 2.6.32-220.7.1.el6.x86_64
2. Intel(R) Xeon(R) CPU E5-2630 v2 @ 2.60GHz
3. gcc version 4.8.2 20140120 (Red Hat 4.8.2-15) (GCC) 


### Test method

We use the test method described at [http://think-async.com/Asio/LinuxPerformanceImprovements](http://think-async.com/Asio/LinuxPerformanceImprovements) using ping-pong protocol to do the throughput benchmark.

Simply to explains that the ping pong protocol is the client and the server both implements the echo protocol. When the TCP connection is established, the client sends some data to the server, the server echoes the data, and then the client echoes to the server again and again. The data will be the same as the table tennis in the client and the server back and forth between the transfer until one side disconnects. This is a common way to test throughput.
 
The test code of [evpp] is at the source code `benchmark/throughput/evpp`, and at here [https://github.com/Qihoo360/evpp/tree/master/benchmark/throughput/evpp](https://github.com/Qihoo360/evpp/tree/master/benchmark/throughput/evpp). We use `tools/benchmark-build.sh` to compile it. The test script is [single_thread.sh](https://github.com/Qihoo360/evpp/blob/master/benchmark/throughput/evpp/single_thread.sh) and [multiple_thread.sh](https://github.com/Qihoo360/evpp/blob/master/benchmark/throughput/evpp/multiple_thread.sh). 

The test code of [asio] is at [https://github.com/huyuguang/asio_benchmark](https://github.com/huyuguang/asio_benchmark) using commits `21fc1357d59644400e72a164627c1be5327fbe3d` and the `client2/server2` test code. The test script is [single_thread.sh](https://github.com/Qihoo360/evpp/blob/master/benchmark/throughput/asio/single_thread.sh) and [multiple_thread.sh](https://github.com/Qihoo360/evpp/blob/master/benchmark/throughput/asio/multiple_thread.sh). 

We have done two benchmarks:

1. Single thread : When the number of concurrent connections is 1/10/100/1000/10000, the message size is 1024/2048/4096/8192/16384/81920.
2. Multi-threaded : When the number of concurrent connections is 100 or 1000, the number of threads in the server and the client is set to 2/3/4/6/8, the message size is 16384 bytes


### Benchmark conclusion

#### Single Thread

1. When concurrency is less than 10,000 in the test, [asio] is better, the average is higher than [evpp] **5% to 10%**
2. When concurrency is 1,10,100,1000 in the test, [evpp]'s performance is better, the average is higher than [asio] **10% ~ 20%**

For details, see the chart below, the horizontal axis is the number of concurrent. The vertical axis is the throughput, the bigger the better.

![](https://raw.githubusercontent.com/zieckey/resources/master/evpp/benchmark/throughput/1thread-evpp-vs-asio-from-huyuguang-1.png)
![](https://raw.githubusercontent.com/zieckey/resources/master/evpp/benchmark/throughput/1thread-evpp-vs-asio-from-huyuguang-2.png)

![](https://raw.githubusercontent.com/zieckey/resources/master/evpp/benchmark/throughput/1thread-evpp-vs-asio-from-huyuguang-1-column.png)
![](https://raw.githubusercontent.com/zieckey/resources/master/evpp/benchmark/throughput/1thread-evpp-vs-asio-from-huyuguang-2-column.png)

#### Multi thread

1. When the number of concurrency is 1000, [evpp] and [asio] have a similar performance and have their own advantages.
2. When the number of concurrency is 100, [asio] is better performing to 10% in this case.

For details, see the chart below. The horizontal axis is the number of threads. The vertical axis is the throughput, the bigger the better.

![](https://raw.githubusercontent.com/zieckey/resources/master/evpp/benchmark/throughput/multi-thread-evpp-vs-asio-from-huyuguang.png)
![](https://raw.githubusercontent.com/zieckey/resources/master/evpp/benchmark/throughput/multi-thread-evpp-vs-asio-from-huyuguang-column.png)

### Analysis

In this ping pong benchmark test, the [asio]'s test code is using a fixed-size buffer to receive and send the data. That can  take advantage of [asio] `Proactor` model, he has almost no memory allocation. Each time [asio] only reads fixed size of data and then sent it out, and then use the same BUFFER to do the next read operation. 

In the same time [evpp] is a network library of `Reactor` model, the receiving data is probably not a fixed size, which involves `evpp::Buffer` internal memory reallocation problems that lead to excessive memory allocation.

We will do another benchmark test to verify the analysis. Please look forward to it.



### Last

The beautiful chart is rendered by [gochart](https://github.com/zieckey/gochart/) 

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