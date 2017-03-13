
#taskset -c 13 build-release/bin/benchmark_ioevent_evpp.noo-space -n 30000 -a 100 -w 30000 &
#taskset -c 15 build-release/bin/benchmark_ioevent_evpp_pipe_watcher.noo-space -n 30000 -a 100 -w 30000 &
#taskset -c 17 build-release/bin/benchmark_ioevent_libevent.noo-space -n 30000 -a 100 -w 30000 &
#taskset -c 19 /home/weizili/git/muduo/pingpong_bench.noo-space -n 30000 -a 100 -w 30000 &
#
#taskset -c 3 build-release/bin/benchmark_ioevent_evpp.has-space -n 30000 -a 100 -w 30000 &
#taskset -c 5 build-release/bin/benchmark_ioevent_evpp_pipe_watcher.has-space -n 30000 -a 100 -w 30000 &
#taskset -c 7 build-release/bin/benchmark_ioevent_libevent.has-space -n 30000 -a 100 -w 30000 &
#taskset -c 9 /home/weizili/git/muduo/pingpong_bench.has-space -n 30000 -a 100 -w 30000 

for num in 500 1000 10000 30000; do
for loop in 1 2 3 4 5 6 7 8 9 10; do
    taskset -c 3 ../../build-release/bin/benchmark_ioevent_evpp_pipe_watcher.has-space -n $num -a 100 -w $num 
    taskset -c 3 ../../build-release/bin/benchmark_ioevent_evpp.has-space -n $num -a 100 -w $num 
    taskset -c 3 ../../build-release/bin/benchmark_ioevent_libevent.has-space -n $num -a 100 -w $num 
    taskset -c 3 ../../../build/release/bin/pingpong_bench.has-space -n $num -a 100 -w $num 
    taskset -c 3 ../../build-release/bin/benchmark_ioevent_evpp_pipe_watcher.noo-space -n $num -a 100 -w $num 
    taskset -c 3 ../../build-release/bin/benchmark_ioevent_evpp.noo-space -n $num -a 100 -w $num 
    taskset -c 3 ../../build-release/bin/benchmark_ioevent_libevent.noo-space -n $num -a 100 -w $num 
    taskset -c 3 ../../../build/release/bin/pingpong_bench.noo-space -n $num -a 100 -w $num 
done
done
