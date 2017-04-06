The performance benchmark of `queue with std::mutex` against `boost::lockfree::queue` and `moodycamel::ConcurrentQueue`

[中文版：基于evpp的EventLoop实现来对无锁队列做一个性能测试对比](https://github.com/Qihoo360/evpp/blob/master/docs/benchmark_lockfree_vs_mutex_cn.md)

### Brief

We can use `EventLoop::QueueInLoop(...)` from [evpp] to execute a task. In one thread, we can use this method to post a task and make this task to be executed in another thread. This is a typical producer/consumer problem.

We can use a queue to store the task. The producer can put tasks to the queue and the consumer takes tasks from queue to execute. In order to avoid thread-safe problems, we need to use a mutex to lock the queue when we modify it, or we can use lock-free mechanism to share data between threads.                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                            

### The testing object

1. [evpp-v0.3.1](https://github.com/Qihoo360/evpp/archive/v0.3.1.zip) based on libevent-2.0.21
2. std::mutex gcc 4.8.2
2. [boost::lockfree::queue from boost-1.58](http://www.boost.org/)
3. [moodycamel::ConcurrentQueue](https://github.com/cameron314/concurrentqueue) with commit c54341183f8674c575913a65ef7c651ecce47243

### Environment

1. Linux CentOS 6.2, 2.6.32-220.7.1.el6.x86_64
2. Intel(R) Xeon(R) CPU E5-2630 v2 @ 2.60GHz
3. gcc version 4.8.2 20140120 (Red Hat 4.8.2-15) (GCC)

### Test method

Test code is at here [https://github.com/Qihoo360/evpp/blob/master/benchmark/post_task/post_task6.cc](https://github.com/Qihoo360/evpp/blob/master/benchmark/post_task/post_task6.cc). The producers post task into the queue the only one consumer to execute. We can specify the count of producer threads and the total count of the tasks posted by every producer. 

The relative code of event_loop.h is bellow:

```C++
    std::shared_ptr<PipeEventWatcher> watcher_;
#ifdef H_HAVE_BOOST
    boost::lockfree::queue<Functor*>* pending_functors_;
#elif defined(H_HAVE_CAMERON314_CONCURRENTQUEUE)
    moodycamel::ConcurrentQueue<Functor>* pending_functors_;
#else
    std::mutex mutex_;
    std::vector<Functor>* pending_functors_; // @Guarded By mutex_
#endif
```

And the relative code of event_loop.cc is bellow:

```C++
void Init() {
    watcher_->Watch(std::bind(&EventLoop::DoPendingFunctors, this));
}

void EventLoop::QueueInLoop(const Functor& cb) {
    {
#ifdef H_HAVE_BOOST
        auto f = new Functor(cb);
        while (!pending_functors_->push(f)) {
        }
#elif defined(H_HAVE_CAMERON314_CONCURRENTQUEUE)
        while (!pending_functors_->enqueue(cb)) {
        }
#else
        std::lock_guard<std::mutex> lock(mutex_);
        pending_functors_->emplace_back(cb);
#endif
    }

    watcher_->Notify();
}

void EventLoop::DoPendingFunctors() {
#ifdef H_HAVE_BOOST
    Functor* f = nullptr;
    while (pending_functors_->pop(f)) {
        (*f)();
        delete f;
    }
#elif defined(H_HAVE_CAMERON314_CONCURRENTQUEUE)
    Functor f;
    while (pending_functors_->try_dequeue(f)) {
        f();
        --pending_functor_count_;
    }
#else
    std::vector<Functor> functors;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        notified_.store(false);
        pending_functors_->swap(functors);
    }
    for (size_t i = 0; i < functors.size(); ++i) {
        functors[i]();
    }
#endif
}
```

We have done two benchmarks:

1. The total count of tasks is 1000000, and the count of producer threads is set to 2/4/6/8/12/16/20
2. The total count of tasks is 1000000, and the count of producer threads is set to 1, and runs 10 times.

### Benchmark conclusion

1. When we have only one producer and only one consumer, most of the time `boost::lockfree::queue` has only a little advantages to `queue with std::mutex` and `moodycamel::ConcurrentQueue`'s performance is the best.
1. When we have multi producers, `boost::lockfree::queue` is better, the average is higher than `queue with std::mutex` about **75%~150%**. `moodycamel::ConcurrentQueue` is the best, the average is higher than `boost::lockfree::queue` about **25%~100%**, and higher than `queue with std::mutex` about **100%~500%**. The more count of producers, the higher performance of `moodycamel::ConcurrentQueue` will get

So we suggest to use `moodycamel::ConcurrentQueue` to exchange datas between threads insdead of `queue with std::mutex` or `boost::lockfree::queue`

For more details, see the chart below, the horizontal axis is the count of producer threads. The vertical axis is the executing time in seconds, lower is better.

![](https://raw.githubusercontent.com/zieckey/resources/master/evpp/benchmark/post_task/boost_lockfree-vs-mutex-1v1.png)
![](https://raw.githubusercontent.com/zieckey/resources/master/evpp/benchmark/post_task/boost_lockfree-vs-mutex.png)

### All benchmark reports

[The IO Event performance benchmark against Boost.Asio](https://github.com/Qihoo360/evpp/blob/master/docs/benchmark_ioevent_performance_vs_asio.md) : [evpp] is higher than [asio] about **20%~50%** in this case

[The ping-pong benchmark against Boost.Asio](https://github.com/Qihoo360/evpp/blob/master/docs/benchmark_ping_pong_spend_time_vs_asio.md) : [evpp] is higher than [asio] about **5%~20%** in this case

[The throughput benchmark against libevent2](https://github.com/Qihoo360/evpp/blob/master/docs/benchmark_throughput_vs_libevent.md) : [evpp] is higher than [libevent] about **17%~130%** in this case 

[The performance benchmark of `queue with std::mutex` against `boost::lockfree::queue` and `moodycamel::ConcurrentQueue`](https://github.com/Qihoo360/evpp/blob/master/docs/benchmark_lockfree_vs_mutex.md) : `moodycamel::ConcurrentQueue` is the best, the average is higher than `boost::lockfree::queue` about **25%~100%** and higher than `queue with std::mutex` about **100%~500%**

[The throughput benchmark against Boost.Asio](https://github.com/Qihoo360/evpp/blob/master/docs/benchmark_throughput_vs_asio.md) : [evpp] and [asio] have the similar performance in this case

[The throughput benchmark against Boost.Asio(中文)](https://github.com/Qihoo360/evpp/blob/master/docs/benchmark_throughput_vs_asio_cn.md) : [evpp] and [asio] have the similar performance in this case

[The throughput benchmark against muduo(中文)](https://github.com/Qihoo360/evpp/blob/master/docs/benchmark_throughput_vs_muduo_cn.md) : [evpp] and [muduo] have the similar performance in this case


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
