#!/bin/sh

set -x
  
SOURCE_DIR=`pwd`
BUILD_TYPE=debug
BUILD_DIR=build-${BUILD_TYPE}

mkdir -p $BUILD_DIR \
  && cd $BUILD_DIR \
  && cmake -DCMAKE_BUILD_TYPE=$BUILD_TYPE $SOURCE_DIR \
  && make -j
