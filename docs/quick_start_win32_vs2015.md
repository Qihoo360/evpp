Quick Start using VS2015
---
	
## Compile evpp on Windows using Microsoft Visual Studio 2015

### Install compiling tool chain

1. `CMake` for windows. You can download it from [https://cmake.org/download/](https://cmake.org/download/)
2. Microsoft `Visual Studio 2015` or higher version. You can download it from [https://www.visualstudio.com/](https://www.visualstudio.com/)
3. Git Bash for windows. You can download it from [https://git-for-windows.github.io/](https://git-for-windows.github.io/)

### Download the source code of evpp

	$ git clone https://github.com/Qihoo360/evpp
	$ cd evpp
	$ git submodule update --init --recursive

### Compile third-party dependent open source code

The `evpp` source is dependent with `libevent`, we suggest you choose the lastest version of libevent. 
Right now, Feb 2017, the latest version of libevent is `2.1.8`.

Go to `3rdparty/libevent-release-2.1.8-stable`

	$ cd 3rdparty/libevent-release-2.1.8-stable
	$ mkdir build && cd build
	$ cmake -G "Visual Studio 14" ..
	$ start libevent.sln
	... # here you can use Visual Studio 2015 to compile the three libevent project event,event_core,event_extra in debug and release mode.
	$
	$ cp lib/Debug/*.* ../../../vsprojects/bin/Debug/
	$ cp lib/Release/*.* ../../../vsprojects/bin/Release/
	$ cp -rf ../include/event2 ../../wininclude/
	$ cp -rf ../build/include/event2/event-config.h ../../wininclude/event2
    $ cd ../../../vsprojects/bin/Debug/
    $ mv libglog_static.lib glog.lib
    $ cd ../Release 
    $ mv libglog_static.lib glog.lib

Note 1: We have modified the source code of libevent-release-2.1.8-stable as bellow:

1. libevent-release-2.1.8-stable/CMakeList.txt : Add 'set(EVENT__DISABLE_OPENSSL ON)' to disable OPENSSL support
2. libevent-release-2.1.8-stable/cmake/VersionViaGit.cmake : Delete or comment the two lines: 'find_package(Git)' and 'include(FindGit)'

### Compile evpp

	$ cd ../../../
	$ start vsprojects/libevpp.sln
	... # here yo can use Visual Studio 2015 to compile the whole evpp project

### Run the unit tests

	$ cd vsprojects/bin/Debug/
	$ ./libevpp-test.exe
