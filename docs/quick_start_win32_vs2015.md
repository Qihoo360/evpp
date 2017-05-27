Quick Start using VS2015
---
	
## Compile evpp on Windows using Microsoft Visual Studio 2015

### Install compiling tool chain

Prerequisites:

- Visual Studio 2015 Update 3 or
- Visual Studio 2017
- CMake 3.8.0 or higher (note: downloaded automatically if not found)
- git.exe available in your path. You can download and install it from [https://git-for-windows.github.io/](https://git-for-windows.github.io/)
- vcpkg. You can download and install it from [https://github.com/Microsoft/vcpkg](https://github.com/Microsoft/vcpkg). Commits c5daa93506b616d253e257488ecc385271238e2a tests OK. Following [https://github.com/Microsoft/vcpkg#quick-start](https://github.com/Microsoft/vcpkg#quick-start) to install [vcpkg](https://github.com/Microsoft/vcpkg). This document assumes that [vcpkg](https://github.com/Microsoft/vcpkg) is installed at `d:\git\vcpkg`. 

### Install dependent libraries by using vcpkg

Use [vcpkg](https://github.com/Microsoft/vcpkg) to install libevent,glog,gtest,gflags.

	D:\git\vcpkg>vcpkg install gflags
	D:\git\vcpkg>vcpkg install glog
	D:\git\vcpkg>vcpkg install libevent-2.x

### Download the source code of evpp

	$ git clone https://github.com/Qihoo360/evpp
	$ cd evpp
	$ git submodule update --init --recursive

### Compile evpp

Using the default vs solution file:

	$ start vsprojects/libevpp.sln
	... # here yo can use Visual Studio 2015 to compile the whole evpp project

Or, we can use CMake to compile the whole projects on WIDNOWS command line console (This does not work on unix shell):

	D:\360.git\evpp>md build
	D:\360.git\evpp>cd build
	D:\360.git\evpp\build>cmake -DCMAKE_TOOLCHAIN_FILE=D:/git/vcpkg/scripts/buildsystems/vcpkg.cmake -G "Visual Studio 14 2015" ..
	D:\360.git\evpp\build>start safe-evpp.sln

### Run the unit tests

	$ cd vsprojects/bin/Debug/
	$ ./libevpp-test.exe
