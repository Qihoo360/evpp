#!/bin/sh

# Run this script at the directory which holds it.

set -x
  
SOURCE_DIR=`pwd`/..
BUILD_TYPE=release
BUILD_DIR=${SOURCE_DIR}/build-${BUILD_TYPE}

mkdir -p $BUILD_DIR \
  && cd $BUILD_DIR \
  && cmake -DCMAKE_BUILD_TYPE=$BUILD_TYPE -DCMAKE_BENCHMARK_TESTING=1 $SOURCE_DIR\
  && make -j\
  && cp -rf $BUILD_DIR/bin/benchmark_pingpong* ../benchmark/throughput/evpp \
  && cp -rf $BUILD_DIR/bin/benchmark_tcp_asio_* ../benchmark/throughput/asio \
  && cp -rf $BUILD_DIR/bin/benchmark_tcp_libevent_* ../benchmark/throughput/libevent \
  && cp -rf $BUILD_DIR/bin/benchmark_ioevent_* ../benchmark/ioevent/libevent \

