#!/bin/sh

set -x
  
SOURCE_DIR=`pwd`
BUILD_TYPE=release
BUILD_DIR=build-${BUILD_TYPE}

rm -rf $BUILD_DIR
mkdir -p $BUILD_DIR \
  && cd $BUILD_DIR \
  && cmake -DCMAKE_BUILD_TYPE=$BUILD_TYPE $SOURCE_DIR \
  && make \
  && make package
