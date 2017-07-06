Quick Start
---

# Dependent open source code

1. libevent
2. glog
3. gtest (optional)
4. boost (optional)
5. gflags (optional)

# How to compile it

### Install compiling tool chain

1. gcc (GCC) 4.8+
2. GNU Make
3. git
4. CMake

### Install dependent open source code

1. Install glog
2. Install libevent-2.x
3. Install gtest
4. Install boost
5. Install gflags

### Download the source code of evpp

	$ git clone https://github.com/Qihoo360/evpp
	$ cd evpp
	$ git submodule update --init --recursive

### Compile and run unit tests.
	
	$ mkdir -p build && cd build
	$ cmake -DCMAKE_BUILD_TYPE=Debug ..
	$ make -j
	$ make test

### Run the examples

	$ cd evpp/build/bin

##### Run a HTTP client example:

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

##### Run a HTTP server example:
	
	$ ./example_httpecho
	WARNING: Logging before InitGoogleLogging() is written to STDERR
	I0306 12:15:31.703927 21228 inner_pre.cc:37] ignore SIGPIPE
	I0306 12:15:31.706221 21228 http_server.cc:99] http server is running

And in another console:

	$ curl "http://127.0.0.1:9009/echo" -d "Hello, world"
	Hello, world

##### Run a TCP echo server example:

	$ ./example_tcpecho

And in another console:

	$ telnet 127.0.0.1 9099 
	Trying 127.0.0.1...
	Connected to 127.0.0.1.
	Escape character is '^]'.

Here we can type any words and we will find it is responsed by our TCP echo server. 

# Other

If you want to compile evpp on Windows using Microsoft Visual Studio 2015, please see [quick_start_windows_with_visualstudio.md](quick_start_windows_with_visualstudio.md)
