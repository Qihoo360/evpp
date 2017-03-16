#!/bin/sh

set -x

killall asio_test.exe
totalcount=${totalcount:-8192}

#Usage: asio_test server3 <address> <port> <threads> <totalcount>
#Usage: asio_test client3 <host> <port> <threads> <totalcount> <sessions>

for nosessions in 100 1000 10000; do
for nothreads in 1 2 3 4 6 8; do
  echo "======================> (test1) TotalCount: $totalcount Threads: $nothreads Sessions: $nosessions"
  sleep 1
  ./asio_test.exe server3 127.0.0.1 33333 $nothreads $totalcount & srvpid=$!
  sleep 1
  ./asio_test.exe client3 127.0.0.1 33333 $nothreads $totalcount  $nosessions 
  sleep 1
  kill -9 $srvpid
  sleep 5
done
done

