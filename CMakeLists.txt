cmake_minimum_required(VERSION 2.8)
project(evpp CXX)

set(CXX_FLAGS
    -g
    -Wall
    -fPIC
    -O3
    -Wall
    -Wno-unused-parameter
    -Wno-unused-function
    -Wunused-variable
    -Wunused-value
    -Wshadow
    -Wcast-qual
    -Wcast-align
    -Wwrite-strings
    -Wsign-compare
    -Winvalid-pch
    -fms-extensions
    -Wfloat-equal
    -Wextra
    -std=c++11
    -DGOOGLE_STRIP_LOG=2
)
string(REPLACE ";" " " CMAKE_CXX_FLAGS "${CXX_FLAGS}")
set(CMAKE_CXX_COMPILER "g++")
set(CMAKE_CXX_FLAGS_DEBUG "-O0")
set(CMAKE_CXX_FLAGS_RELEASE "-O2 -finline-limit=1000 -DNDEBUG")

message(STATUS "This is BINARY dir " ${PROJECT_BINARY_DIR})
message(STATUS "This is SOURCE dir " ${PROJECT_SOURCE_DIR})
message(STATUS "CXX_FLAGS " ${CXX_FLAGS})

add_subdirectory(evpp lib)
