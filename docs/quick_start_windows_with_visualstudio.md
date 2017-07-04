Quick Start of using VS2015
---
	
## Compile evpp from source code on Windows using Microsoft Visual Studio 2015

#### Install compiling tool chain

Prerequisites:

- Visual Studio 2015 Update 3 or
- Visual Studio 2017
- CMake 3.8.0  (note: downloaded automatically if not found, but version must be 3.8.x)
- git.exe available in your path. You can download and install it from [https://git-for-windows.github.io/]
- vcpkg. You can download and install it from [https://github.com/Microsoft/vcpkg]. Commits c5daa93506b616d253e257488ecc385271238e2a tests OK. Following [https://github.com/Microsoft/vcpkg#quick-start](https://github.com/Microsoft/vcpkg#quick-start) to install [vcpkg]. This document assumes that [vcpkg] is installed at `d:\git\vcpkg`.

#### Install dependent libraries by using vcpkg

Use [vcpkg] to install libevent,glog,gtest,gflags.

##### for win_x32:
	
	D:\git\vcpkg>vcpkg install gflags
	D:\git\vcpkg>vcpkg install glog
	D:\git\vcpkg>vcpkg install openssl
	D:\git\vcpkg>vcpkg install libevent

##### for win_x64:
	
	D:\git\vcpkg>vcpkg install gflags:x64-windows
	D:\git\vcpkg>vcpkg install glog:x64-windows
	D:\git\vcpkg>vcpkg install openssl:x64-windows
	D:\git\vcpkg>vcpkg install libevent:x64-windows


#### Download the source code of evpp

	$ git clone https://github.com/Qihoo360/evpp
	$ cd evpp
	$ git submodule update --init --recursive

#### Compile evpp

Using the default vs solution file:

	$ start vsprojects/libevpp.sln
	... # here yo can use Visual Studio 2015 to compile the whole evpp project

Or, we can use CMake to compile the whole projects on WIDNOWS command line console (This does not work on unix shell):

##### for win_x32:
	D:\git\evpp>md build
	D:\git\evpp>cd build
	D:\git\evpp\build>cmake -DCMAKE_TOOLCHAIN_FILE=your_vcpkg_path/scripts/buildsystems/vcpkg.cmake -G "Visual Studio 14 2015" ..
	D:\git\evpp\build>start safe-evpp.sln

##### for win_x64:
	D:\git\evpp>md build
	D:\git\evpp>cd build
	D:\git\evpp\build>cmake -DCMAKE_TOOLCHAIN_FILE=your_vcpkg_path/scripts/buildsystems/vcpkg.cmake -G "Visual Studio 14 2015 Win64" ..
	D:\git\evpp\build>start safe-evpp.sln

#### Run the unit tests

	$ cd vsprojects/bin/Debug/
	$ ./libevpp-test.exe

## Use evpp as a library

If you just want to use [evpp] as a library, you can use [vcpkg] to install [evpp]:

	 D:\git\vcpkg>vcpkg install evpp

That will install [evpp] in your local machine. And then, you can use [evpp] in you own applications.


[evpp]:https://github.com/Qihoo360/evpp
[https://github.com/Microsoft/vcpkg]:https://github.com/Microsoft/vcpkg
[vcpkg]:https://github.com/Microsoft/vcpkg
[https://git-for-windows.github.io/]:https://git-for-windows.github.io/






