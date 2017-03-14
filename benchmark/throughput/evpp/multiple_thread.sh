#!/bin/sh

killall benchmark_pingpong_server
timeout=${timeout:-100}
bufsize=${bufsize:-16384}

for nosessions in 100 1000; do
  for nothreads in 1 2 3 4; do
    sleep 5
    echo "Bufsize: $bufsize Threads: $nothreads Sessions: $nosessions"
    ./benchmark_pingpong_server 33333 $nothreads & srvpid=$!
    sleep 1
    ./benchmark_pingpong_client 127.0.0.1 33333 $nothreads $bufsize $nosessions $timeout
    kill -9 $srvpid
  done
done
