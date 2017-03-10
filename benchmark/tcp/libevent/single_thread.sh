#!/bin/sh

killall server
timeout=${timeout:-100}
bufsize=${bufsize:-16384}
nothreads=1

for nosessions in 1 10 100 1000 10000; do
  sleep 5
  echo "Bufsize: $bufsize Threads: $nothreads Sessions: $nosessions"
  taskset -c 1 ./server 2> /dev/null & srvpid=$!
  sleep 1
  taskset -c 2 ./client 9876 $bufsize $nosessions $timeout
  kill -9 $srvpid
done

