#pragma once

#include <thread>
#include <mutex>

#include "evpp/inner_pre.h"
#include "evpp/server_status.h"

struct event_base;
struct event;

namespace evpp {

class EventLoop;

class EVPP_EXPORT EventLoopThread : public ServerStatus {
public:
    enum { kOK = 0 };

    // Return 0 means OK, anything else means failed.
    typedef std::function<int()> Functor;

    EventLoopThread();
    ~EventLoopThread();

    // @param wait_thread_started - If it is true this method will block
    //  until the thread totally started
    // @param pre - This functor will be executed immediately when the thread is started. 
    // @param post - This functor will be executed at the moment when the thread is going to stop.
    bool Start(bool wait_thread_started = true,
               Functor pre = Functor(),
               Functor post = Functor());

    void Stop(bool wait_thread_exit = false);

    // @brief Join the thread. If you forget to call this method,
    // it will be invoked automatically in the destruct method ~EventLoopThread().
    // @note DO NOT call this method from any of the working thread.
    void Join();

    void AfterFork();

public:
    void set_name(const std::string& n);
    const std::string& name() const;
    EventLoop* loop() const;
    struct event_base* event_base();
    std::thread::id tid() const;
    bool IsRunning() const;

private:
    void Run(const Functor& pre, const Functor& post);

private:
    std::shared_ptr<EventLoop> event_loop_;

    std::mutex mutex_;
    std::shared_ptr<std::thread> thread_; // Guard by mutex_

    std::string name_;
};
}
