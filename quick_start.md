Quick Start
---

# Dependent open source code

1. libevent
2. glog 

# How to compile it

## Compile evpp on Linux

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
	$ make
	$ cd test
	$ make
	$ ./evpp-test
	
## Compile evpp on Windows using Microsoft Visual Studio 2015

### Install compiling tool chain

1. `CMake` for windows. You can download it from [https://cmake.org/download/](https://cmake.org/download/)
2. Microsoft `Visual Studio 2015` or higher version. You can download it from [https://www.visualstudio.com/](https://www.visualstudio.com/)
3. Git Bash for windows. You can download it from [https://git-for-windows.github.io/](https://git-for-windows.github.io/)

### Download the source code of evpp

	$ git clone https://github.com/Qihoo360/evpp

### Compile third-party dependent open source code

The `evpp` source is dependent with `libevent`, we suggest you choose the lastest version of libevent. 
Right now, Jan 2017, the latest version of libevent is `2.1.7-rc`.

Go to `evpp/3rdparty/libevent-release-2.1.7-rc`

	$ cd evpp/3rdparty/libevent-release-2.1.7-rc
	$ mkdir build && cd build
	$ cmake -G "Visual Studio 14" ..
	$ start libevent.sln
	... # here you can use Visual Studio 2015 to compile the three libevent project event,event_core,event_extra in debug and release mode.
	$ cd ../../
	$ cp libevent-release-2.1.7-rc/build/lib/Debug/*.* ../msvc/bin/Debug/
	$ cp libevent-release-2.1.7-rc/build/lib/Release/*.* ../msvc/bin/Release/
	$ cp -rf libevent-release-2.1.7-rc/include/event2 wininclude/
	$ cp -rf libevent-release-2.1.7-rc/build/include/event2/event-config.h wininclude/event2

Note: We have modified the source code of libevent-release-2.1.7-rc as bellow:
	libevent-release-2.1.7-rc/CMakeList.txt : Add 'set(EVENT__DISABLE_OPENSSL 1)' to disable OPENSSL support
	libevent-release-2.1.7-rc/cmake/VersionViaGit.cmake : Comments '#find_package(Git)' and '#include(FindGit)'

### Compile evpp

	$ cd ../
	$ start msvc/libevpp.sln
	... # here yo can use Visual Studio 2015 to compile the whole evpp project

### Run the unit tests

	$ cd msvc/bin/Debug/
	$ ./libevpp-test.exe
