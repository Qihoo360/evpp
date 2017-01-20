evpp
---

`evpp` is a modern C++ network library for develop high performance network servers in TCP/UDP/HTTP protocols.
`evpp` provides a TCP Server to support multi-threaded nonblocking server. And it also provides a HTTP and UDP Server to support http and udp prococol.

`evpp` provides:

1. A nonblocking TCP server
1. A nonblocking TCP client
1. A nonblocking HTTP server
1. A nonblocking HTTP client
1. A blocking UDP server
1. EventLoop
1. Thread pool
1. Timer


# Dependency open source code

1. libevent
2. glog 

# How to compile it

## Compile evpp on Windows using Microsoft Visual Studio 2015

### Install compiling tool chain
1. `CMake` for windows. You can download it from [https://cmake.org/download/](https://cmake.org/download/)
2. Microsoft `Visual Studio 2015` or higher version. You can download it from [https://www.visualstudio.com/](https://www.visualstudio.com/)

### Download the source of evpp

	$ git clone https://github.com/Qihoo360/evpp

### Compile third-party dependent open source code

The `evpp` source is dependent with `libevent`, we suggest you choose the lastest version of libevent. 
Right now, Jan 2017, the latest version of libevent is `2.1.7-rc`.

Go to `evpp/3rdparty/libevent-release-2.1.7-rc`

	$ cd evpp/3rdparty/libevent-release-2.1.7-rc
	$ vim CMakeList.txt # Add 'set(EVENT__DISABLE_OPENSSL 1)' to disable OPENSSL support
	$ md build && cd build
	$ cmake -G "Visual Studio 14" ..
	$ start libevent.sln
	... # here you can use Visual Studio 2015 to compile the three libevent project event,event_core,event_extra in debug and release mode.
	$ cd ../../
	$ cp libevent-release-2.1.7-rc/build/lib/Debug/*.lib ../msvc/bin/Debug/
	$ cp libevent-release-2.1.7-rc/build/lib/Release/*.lib ../msvc/bin/Release/
	$ cp -rf libevent-release-2.1.7-rc/include/event2 wininclude/
	$ cp -rf libevent-release-2.1.7-rc/build/include/event2/event-config.h wininclude/event2

## Compile evpp on Linux

### Install compiling tool chain
1. gcc (GCC) 4.8+
2. GNU Make 3.8

### Download the source of evpp

	$ git clone https://github.com/Qihoo360/evpp

### Compile third-party dependent open source code


# Thanks

Thanks for the support of [Qihoo360](http://www.360.cn "http://www.360.cn").

Thanks for `libevent`, `glog`, `gtest` projects. There are great open source projects.

`evpp` is highly inspired by [muduo](https://github.com/chenshuo/muduo "https://github.com/chenshuo/muduo"). Thanks for the great work of [Chen Shuo](https://github.com/chenshuo "https://github.com/chenshuo")

