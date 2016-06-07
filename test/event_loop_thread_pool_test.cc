#include "test_common.h"

#include "evpp/libevent_headers.h"
#include "evpp/libevent_watcher.h"
#include "evpp/event_loop.h"
#include "evpp/event_loop_thread_pool.h"

#include <atomic>
#include <thread>
#include <mutex>

#include <set>

namespace
{
    static std::set<std::thread::id> g_working_tids;

    static void OnWorkingThread()
    {
        static std::mutex mutex;
        std::lock_guard<std::mutex> g(mutex);
        g_working_tids.insert(std::this_thread::get_id());
    }


    static std::atomic<int> g_count;
    static void OnCount()
    {
        g_count++;
        OnWorkingThread();
    }
}

TEST_UNIT(testEventLoopThreadPool)
{
    evpp::EventLoopThread loop;
    loop.Start(true);

    int thread_num = 24;
    evpp::EventLoopThreadPool pool(loop.event_loop());
    pool.SetThreadNum(thread_num);
    H_TEST_ASSERT(pool.Start(true));

    for (int i = 0; i < thread_num; i++)
    {
        pool.GetNextLoop()->RunInLoop(&OnCount);
    }

    usleep(1000*1000);
    pool.Stop(true);
    loop.Stop(true);
    usleep(2000 * 1000);
    H_TEST_ASSERT((int)g_working_tids.size() == thread_num);
}

