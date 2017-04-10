#!/bin/sh

# Run this script at the directory which holds it.

set -x
  
SOURCE_DIR=`pwd`/..
BUILD_TYPE=debug
BUILD_DIR=${SOURCE_DIR}/build-${BUILD_TYPE}

mkdir -p $BUILD_DIR \
  && cd $BUILD_DIR \
  && cmake -DCMAKE_BUILD_TYPE=$BUILD_TYPE $SOURCE_DIR \
  && make -j 
