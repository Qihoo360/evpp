
for num in 500 1000 10000 30000; do
for loop in 1 2 3 4 5 6 7 8 9 10; do
    taskset -c 3 ../../build-release/bin/benchmark_ioevent_evpp_pipe_watcher -n $num -a 100 -w $num 
    taskset -c 3 ../../build-release/bin/benchmark_ioevent_evpp -n $num -a 100 -w $num 
    taskset -c 3 ../../build-release/bin/benchmark_ioevent_libevent -n $num -a 100 -w $num 
    taskset -c 3 ../../../build/release/bin/pingpong_bench -n $num -a 100 -w $num 
done
done
