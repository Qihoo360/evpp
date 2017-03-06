Quick Start
---

# Dependent open source code

1. libevent
2. glog
3. gtest

# How to compile it

### Install compiling tool chain
1. gcc (GCC) 4.8+
2. GNU Make

### Install dependent open source code

1. Install glog
2. Install libevent 2.x

### Download the source code of evpp

	$ git clone https://github.com/Qihoo360/evpp

### Compile and run unit tests.
	
	$ cd evpp
	$ mkdir -p build && cd build
	$ cmake -DCMAKE_BUILD_TYPE=Debug ..
	$ make -j
	$ make test

### Run the examples

	$ cd evpp/build/bin

Run a HTTP client example:

	$ ./example_http_client_request01
	WARNING: Logging before InitGoogleLogging() is written to STDERR
	I0306 11:45:09.464159 13230 inner_pre.cc:37] ignore SIGPIPE
	I0306 11:45:09.464896 13230 client01.cc:30] Do http request
	I0306 11:45:09.493073 13231 client01.cc:14] http_code=200 [ok
	]
	I0306 11:45:09.493124 13231 client01.cc:16] HTTP HEADER Connection=close
	I0306 11:45:09.493242 13231 event_loop.cc:103] EventLoop is stopping now, tid=140722561709824
	I0306 11:45:09.993921 13231 event_loop.cc:93] EventLoop stopped, tid: 140722561709824
	I0306 11:45:09.994107 13230 client01.cc:38] EventLoopThread stopped. 

Run a HTTP server example:
	
	$ ./example_httpecho
	WARNING: Logging before InitGoogleLogging() is written to STDERR
	I0306 12:15:31.703927 21228 inner_pre.cc:37] ignore SIGPIPE
	I0306 12:15:31.706221 21228 http_server.cc:99] http server is running

And in another console:

	$ curl "http://127.0.0.1:9009/echo" -d "Hello, world"
	Hello, world

### Other

If you want to compile evpp on Windows using Microsoft Visual Studio 2015, please see [quick_start_win32_vs2015.md](quick_start_win32_vs2015.md)
