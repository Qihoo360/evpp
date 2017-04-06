基于evpp的EventLoop实现来对无锁队列做一个性能测试对比

[English version : The performance benchmark of `queue with std::mutex` against `boost::lockfree::queue` and `moodycamel::ConcurrentQueue`](https://github.com/Qihoo360/evpp/blob/master/docs/benchmark_lockfree_vs_mutex_cn.md)

### Brief

我们使用[https://github.com/Qihoo360/evpp]项目中的`EventLoop::QueueInLoop(...)`函数来做这个性能测试。我们通过该函数能够将一个仿函数执行体从一个线程调度到另一个线程中执行。这是一个典型的生产者和消费者问题。

我们用一个队列来保存这种仿函数执行体。多个生产者线程向这个队列写入仿函数执行体，一个消费者线程从队列中取出仿函数执行体来执行。为了保证队列的线程安全问题，我们可以使用一个锁来保护这个队列，或者使用无锁队列机制来解决安全问题。`EventLoop::QueueInLoop(...)`函数通过通定义实现了三种不同模式的跨线程交换数据的队列。                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                        

### 测试对象

1. [evpp-v0.3.2](https://github.com/Qihoo360/evpp/archive/v0.3.2.zip)
2. `EventLoop::QueueInLoop(...)`函数内的队列的三种实现方式：
	- 带锁的队列用`std::vector`和`std::mutex`来实现，具体的 gcc 版本为 4.8.2
	- [boost::lockfree::queue from boost-1.53](http://www.boost.org/)
	- [moodycamel::ConcurrentQueue](https://github.com/cameron314/concurrentqueue) with commit c54341183f8674c575913a65ef7c651ecce47243

### 测试环境

1. Linux CentOS 6.2, 2.6.32-220.7.1.el6.x86_64
2. Intel(R) Xeon(R) CPU E5-2630 v2 @ 2.60GHz
3. gcc version 4.8.2 20140120 (Red Hat 4.8.2-15) (GCC)

### 测试方法

测试代码请参考[https://github.com/Qihoo360/evpp/blob/master/benchmark/post_task/post_task6.cc](https://github.com/Qihoo360/evpp/blob/master/benchmark/post_task/post_task6.cc). 在一个消费者线程中运行一个`EventLoop`对象`loop_`，多个生产者线程不停的调用`loop_->QueueInLoop(...)`方法将仿函数执行体放入到消费者的队列中让其消费（执行）。每个生产者线程放入一定总数（由运行参数指定）的仿函数执行体之后就停下来，等消费者线程完全消费完所有的仿函数执行体之后，程序退出，并记录开始和结束时间。

为了便于大家阅读，现将相关代码的核心部分摘录如下。

event_loop.h中定义了队列：

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

event_loop.cc中定义了`QueueInLoop(...)`的具体实现：

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

我们进行了两种测试：

1. 一个生产者线程投递1000000个仿函数执行体到消费者线程中执行，统计总耗时。然后同样的方法我们反复测试10次
1. 生产者线程分别是2/4/6/8/12/16/20，每个线程投递1000000个仿函数执行体到消费者线程中执行，并统计总共耗时


### 测试结论

1. 当我们只有生产者和消费者都只有一个时，大多数测试结果表明`moodycamel::ConcurrentQueue`的性能是最好的，大概比`queue with std::mutex`高出**10%~50%**左右的性能。`boost::lockfree::queue`比`queue with std::mutex`的性能只能高出一点点。由于我们的实现中，必须要求能够使用多生产者的写入，所以并没有测试boost中专门的单生产者单消费者的无锁队列`boost::lockfree::spsc_queue`，在这种场景下，boost稍稍有些吃亏，但并不影响整体测试结果及结论。
1. 当我们有多个生产者线程和一个消费者线程时，`boost::lockfree::queue`的性能比`queue with std::mutex`高出**75%~150%**左右。 `moodycamel::ConcurrentQueue`的性能最好，大概比`boost::lockfree::queue`高出**25%~100%**，比`queue with std::mutex`高出**100%~500%**。当生产者线程越多，也就是锁冲突概率越大时，`moodycamel::ConcurrentQueue`的性能优势体现得更加明显。

因此，上述对比测试结论，就我们的[evpp]项目中的`EventLoop`的实现方式，我们推荐使用`moodycamel::ConcurrentQueue`来实现跨线程的数据交换。

更详细的测试数据，请参考下面的两个图表。

纵轴是执行耗时，越低性能越高。

图1，生产者和消费者都只有一个，横轴是测试的批次：
![](https://raw.githubusercontent.com/zieckey/resources/master/evpp/benchmark/post_task/boost_lockfree-vs-mutex-1v1.png)

图2，生产者线程有多个，横轴是生产者线程的个数，分别是2/4/6/8/12/16/20：
![](https://raw.githubusercontent.com/zieckey/resources/master/evpp/benchmark/post_task/boost_lockfree-vs-mutex.png)

### 其他的性能测试报告

[The IO Event performance benchmark against Boost.Asio](https://github.com/Qihoo360/evpp/blob/master/docs/benchmark_ioevent_performance_vs_asio.md) : [evpp] is higher than [asio] about **20%~50%** in this case

[The ping-pong benchmark against Boost.Asio](https://github.com/Qihoo360/evpp/blob/master/docs/benchmark_ping_pong_spend_time_vs_asio.md) : [evpp] is higher than [asio] about **5%~20%** in this case

[The throughput benchmark against libevent2](https://github.com/Qihoo360/evpp/blob/master/docs/benchmark_throughput_vs_libevent.md) : [evpp] is higher than [libevent] about **17%~130%** in this case 

[The performance benchmark of `queue with std::mutex` against `boost::lockfree::queue` and `moodycamel::ConcurrentQueue`](https://github.com/Qihoo360/evpp/blob/master/docs/benchmark_lockfree_vs_mutex.md) : `moodycamel::ConcurrentQueue` is the best, the average is higher than `boost::lockfree::queue` about **25%~100%** and higher than `queue with std::mutex` about **100%~500%**

[The throughput benchmark against Boost.Asio](https://github.com/Qihoo360/evpp/blob/master/docs/benchmark_throughput_vs_asio.md) : [evpp] and [asio] have the similar performance in this case

[The throughput benchmark against Boost.Asio(中文)](https://github.com/Qihoo360/evpp/blob/master/docs/benchmark_throughput_vs_asio_cn.md) : [evpp] and [asio] have the similar performance in this case

[The throughput benchmark against muduo(中文)](https://github.com/Qihoo360/evpp/blob/master/docs/benchmark_throughput_vs_muduo_cn.md) : [evpp] and [muduo] have the similar performance in this case


### 最后

报告中的图表是使用[gochart]绘制的。

非常感谢您的阅读。如果您有任何疑问，请随时在[issue](https://github.com/Qihoo360/evpp/issues)跟我们讨论。谢谢。

[Boost.Asio]:http://www.boost.org/
[boost.asio]:http://www.boost.org/
[asio]:http://www.boost.org/
[boost]:http://www.boost.org/
[evpp]:https://github.com/Qihoo360/evpp
[https://github.com/Qihoo360/evpp]:https://github.com/Qihoo360/evpp
[muduo]:https://github.com/chenshuo/muduo
[libevent2]:https://github.com/libevent/libevent
[libevent]:https://github.com/libevent/libevent
[Golang]:https://golang.org
[Buffer]:https://github.com/Qihoo360/evpp/blob/master/evpp/buffer.h
[recipes]:https://github.com/chenshuo/recipes
[gochart]:https://github.com/zieckey/gochart/
