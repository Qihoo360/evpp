Quick Start
---

# Dependent open source code

1. libevent
2. glog 

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
	$ cmake ..
	$ make -j
	$ make test
	
### Other

If you want to compile evpp on Windows using Microsoft Visual Studio 2015, please see [docs/quick_start_win32_vs2015.md](docs/quick_start_win32_vs2015.md)
