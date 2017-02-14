cmake_minimum_required (VERSION 2.8)

project (safe-evpp C CXX)

if (UNIX)
set (CXX_FLAGS
    -g
    -O3
    -fPIC
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
include_directories (${PROJECT_SOURCE_DIR}
				  	 ${PROJECT_SOURCE_DIR}/3rdparty)
else (UNIX)
set (CXX_FLAGS -DWIN32 -D_WIN32 -EHsc)
include_directories(${PROJECT_SOURCE_DIR}
					${PROJECT_SOURCE_DIR}/3rdparty/libevent-release-2.1.7-rc/include
					${PROJECT_SOURCE_DIR}/3rdparty/libevent-release-2.1.7-rc/build/include
                    ${PROJECT_SOURCE_DIR}/3rdparty/glog-0.3.4/src/windows
                    ${PROJECT_SOURCE_DIR}/3rdparty/wininclude)
endif (UNIX)

string (REPLACE ";" " " CMAKE_CXX_FLAGS "${CXX_FLAGS}")

message (STATUS "This is BINARY dir " ${PROJECT_BINARY_DIR})
message (STATUS "This is SOURCE dir " ${PROJECT_SOURCE_DIR})
message (STATUS "CXX_FLAGS " ${CXX_FLAGS})

set (EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)
set (LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)

add_subdirectory (evpp)
add_subdirectory (apps)
add_subdirectory (test)
add_subdirectory (examples)

set (CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")
include (utils)
include (packages)

include (CPack)
include (CTest)
