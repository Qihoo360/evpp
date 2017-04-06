
for count in 1000000; do
for thread in 1 2 4 6 8 12 16 20; do
    ../../build-release/bin/benchmark_post_task1 $thread $count
    ../../build-release/bin/benchmark_post_task_boost_lockfree_queue_queue1 $thread $count
    ../../build-release/bin/benchmark_post_task2 $thread $count
    ../../build-release/bin/benchmark_post_task_boost_lockfree_queue_queue2 $thread $count
    ../../build-release/bin/benchmark_post_task6 $thread $count
    ../../build-release/bin/benchmark_post_task_boost_lockfree_queue_queue6 $thread $count
    ../../build-release/bin/benchmark_post_task_concurrentqueue6 $thread $count
done
    ../../build-release/bin/benchmark_post_task3 $count
    ../../build-release/bin/benchmark_post_task_boost_lockfree_queue_queue3 $count
    ../../build-release/bin/benchmark_post_task4 $count
    ../../build-release/bin/benchmark_post_task_boost_lockfree_queue_queue4 $count
    ../../build-release/bin/benchmark_post_task5 $count
    ../../build-release/bin/benchmark_post_task_boost_lockfree_queue_queue5 $count
done
