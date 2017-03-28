
for count in 10000 100000 1000000; do
for thread in 2 4 6 8 12 16 20; do
    ../../build-release/bin/benchmark_post_task1 $thread $count
    ../../build-release/bin/benchmark_post_lockfree_task1 $thread $count
    ../../build-release/bin/benchmark_post_task2 $thread $count
    ../../build-release/bin/benchmark_post_lockfree_task2 $thread $count
    ../../build-release/bin/benchmark_post_task6 $thread $count
    ../../build-release/bin/benchmark_post_lockfree_task6 $thread $count
done
    ../../build-release/bin/benchmark_post_task3 $count
    ../../build-release/bin/benchmark_post_lockfree_task3 $count
    ../../build-release/bin/benchmark_post_task4 $count
    ../../build-release/bin/benchmark_post_lockfree_task4 $count
    ../../build-release/bin/benchmark_post_task5 $count
    ../../build-release/bin/benchmark_post_lockfree_task5 $count
done