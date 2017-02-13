cmake_minimum_required(VERSION 2.8)

set (CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include (utils)

# ----------------------------------------------------------------------------
# package information
set (PACKAGE_NAME        "evpp")
set (PACKAGE_VERSION     "1.0.0")
set (PACKAGE_STRING      "${PACKAGE_NAME} ${PACKAGE_VERSION}")
set (PACKAGE_TARNAME     "${PACKAGE_NAME}-${PACKAGE_VERSION}")
set (PACKAGE_BUGREPORT   "https://github.com/Qihoo360/evpp/issues")
set (PACKAGE_DESCRIPTION "evpp is a modern C++ network library for developing high performance network servers in TCP/UDP/HTTP protocols.")
set (PACKAGE_URL         "http://github.com/Qihoo360/evpp")

project (${PACKAGE_NAME} CXX)
if (CMAKE_VERSION VERSION_LESS 100)
  # C language still needed because the following required CMake modules
  # (or their dependencies, respectively) are not correctly handling
  # the case where only CXX is enabled.
  # - CheckTypeSize.cmake (fixed in CMake 3.1, cf. http://www.cmake.org/Bug/view.php?id=14056)
  # - FindThreads.cmake (--> CheckIncludeFiles.cmake <--)
  enable_language (C)
endif ()

version_numbers (
  ${PACKAGE_VERSION}
    PACKAGE_VERSION_MAJOR
    PACKAGE_VERSION_MINOR
    PACKAGE_VERSION_PATCH
)
set (PACKAGE_SOVERSION "${PACKAGE_VERSION_MAJOR}.${PACKAGE_VERSION_MINOR}")

include (CPack)
include (CTest)





project(${PACKAGE_NAME} CXX)

if (UNIX)
set(CXX_FLAGS
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
include_directories(${PROJECT_SOURCE_DIR}
					${PROJECT_SOURCE_DIR}/3rdparty)
else(UNIX)
set(CXX_FLAGS -DWIN32 -D_WIN32 -EHsc)
include_directories(${PROJECT_SOURCE_DIR}
					${PROJECT_SOURCE_DIR}/3rdparty/libevent-release-2.1.7-rc/include
					${PROJECT_SOURCE_DIR}/3rdparty/libevent-release-2.1.7-rc/build/include
                    ${PROJECT_SOURCE_DIR}/3rdparty/glog-0.3.4/src/windows
                    ${PROJECT_SOURCE_DIR}/3rdparty/wininclude)
endif(UNIX)

string(REPLACE ";" " " CMAKE_CXX_FLAGS "${CXX_FLAGS}")

message(STATUS "This is BINARY dir " ${PROJECT_BINARY_DIR})
message(STATUS "This is SOURCE dir " ${PROJECT_SOURCE_DIR})
message(STATUS "CXX_FLAGS " ${CXX_FLAGS})

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)

add_subdirectory(evpp)
add_subdirectory(apps)
add_subdirectory(test)
add_subdirectory(examples)




#install (
#  TARGETS evpp ${PACKAGE_NAME}
#  #EXPORT ${PACKAGE_NAME} 
#  #RUNTIME DESTINATION bin
#  #PUBLIC_HEADER DESTINATION include/glog
#  LIBRARY DESTINATION lib
#  ARCHIVE DESTINATION lib)


