#!/bin/sh

set -x

killall benchmark_pingpong_server
timeout=${timeout:-100}
#bufsize=${bufsize:-16384}
nothreads=1

for bufsize in 1024 2048 4096 8192 16384 81920 409600; do
for nosessions in 1 10 100 1000 10000; do
  echo "======================> Bufsize: $bufsize Threads: $nothreads Sessions: $nosessions"
  taskset -c 1 ./benchmark_pingpong_server 33333 $nothreads & srvpid=$!
  sleep 1
  taskset -c 2 ./benchmark_pingpong_client 127.0.0.1 33333 $nothreads $bufsize $nosessions $timeout
  sleep 1
  taskset -c 2 ./benchmark_pingpong_client_fixed_size 127.0.0.1 33333 $nothreads $bufsize $nosessions $timeout
  kill -9 $srvpid
  sleep 5
done
done
