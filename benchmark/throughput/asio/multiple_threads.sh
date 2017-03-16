#!/bin/sh

#set -x

killall asio_test.exe
timeout=${timeout:-10}
bufsize=${bufsize:-16384}
nothreads=1

for nosessions in 100 1000; do
for nothreads in 2 3 4 6 8; do
  echo "======================> (test1) Bufsize: $bufsize Threads: $nothreads Sessions: $nosessions"
  sleep 1
  ./asio_test.exe server1 127.0.0.1 33333 $nothreads $bufsize & srvpid=$!
  sleep 1
  ./asio_test.exe client1 127.0.0.1 33333 $nothreads $bufsize $nosessions $timeout
  sleep 1
  kill -9 $srvpid
  sleep 5
done
done

for nosessions in 100 1000; do
for nothreads in 2 3 4 6 8; do
  echo "======================> (test2) Bufsize: $bufsize Threads: $nothreads Sessions: $nosessions"
  sleep 1
  ./asio_test.exe server2 127.0.0.1 33333 $nothreads $bufsize & srvpid=$!
  sleep 1
  ./asio_test.exe client2 127.0.0.1 33333 $nothreads $bufsize $nosessions $timeout
  sleep 1
  kill -9 $srvpid
  sleep 5
done
done
