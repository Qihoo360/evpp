#!/bin/sh

set -x

killall asio_test.exe
totalcount=${totalcount:-8192}

for nosessions in 100 1000 10000; do
for nothreads in 1 2 3 4 6 8; do
  echo "======================> (test1) TotalCount: $totalcount Threads: $nothreads Sessions: $nosessions"
  sleep 1
  ../../../build-release/bin/benchmark_pingpong_header_body_server 6666 $nothreads $totalcount & srvpid=$!
  sleep 1
  ../../../build-release/bin/benchmark_pingpong_header_body_client 127.0.0.1 6666 $nothreads $totalcount $nosessions 
  sleep 1
  kill -9 $srvpid
  sleep 5
done
done

