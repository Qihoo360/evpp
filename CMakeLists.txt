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

# ----------------------------------------------------------------------------
# package information
# TODO add el7.x86_64 
execute_process (
            COMMAND         ${PROJECT_SOURCE_DIR}/cmake/rpm_kernel_release.sh
            RESULT_VARIABLE RV
            OUTPUT_VARIABLE CENTOS_VERSION
            )
if (RV EQUAL 0)
set (KERNEL_RELEASE ${CENTOS_VERSION})
else ()
set (KERNEL_RELEASE "unknown-arch")
endif ()

execute_process (
            COMMAND         ${PROJECT_SOURCE_DIR}/cmake/git_checkin_count.sh
            RESULT_VARIABLE RV
            OUTPUT_VARIABLE GIT_CHECKIN_COUNT
            )
if (RV EQUAL 0)
else ()
set (GIT_CHECKIN_COUNT "1")
endif ()

set (PACKAGE_NAME        "${CMAKE_PROJECT_NAME}")
set (PACKAGE_VERSION     "1.0.0")
set (PACKAGE_STRING      "${PACKAGE_NAME} ${PACKAGE_VERSION}")
set (PACKAGE_TARNAME     "${PACKAGE_NAME}-${PACKAGE_VERSION}")
set (PACKAGE_BUGREPORT   "https://github.com/Qihoo360/evpp/issues")
set (PACKAGE_DESCRIPTION "evpp is a modern C++ network library for developing high performance network servers in TCP/UDP/HTTP protocols.")
set (PACKAGE_URL         "http://github.com/Qihoo360/evpp")

version_numbers (
  ${PACKAGE_VERSION}
    PACKAGE_VERSION_MAJOR
    PACKAGE_VERSION_MINOR
    PACKAGE_VERSION_PATCH
)
set (PACKAGE_SOVERSION "${PACKAGE_VERSION_MAJOR}.${PACKAGE_VERSION_MINOR}")

set (CPACK_PACKAGE_CONTACT       "https://github.com/Qihoo360/evpp")
set (CPACK_PACKAGE_ARCHITECTURE  "${KERNEL_RELEASE}")
set (CPACK_PACKAGE_VERSION_MAJOR "${PACKAGE_VERSION_MAJOR}")
set (CPACK_PACKAGE_VERSION_MINOR "${PACKAGE_VERSION_MINOR}")
set (CPACK_PACKAGE_VERSION_PATCH "${PACKAGE_VERSION_PATCH}")
set (CPACK_PACKAGE_FILE_NAME     "${PACKAGE_NAME}-${PACKAGE_VERSION}-${GIT_CHECKIN_COUNT}.${CPACK_PACKAGE_ARCHITECTURE}")
set (CPACK_OUTPUT_FILE_PREFIX   packages)

#message(STATUS "XXXXXXXXXXXXXXX PACKAGE_SOVERSION=" ${PACKAGE_SOVERSION})
#message(STATUS "XXXXXXXXXXXXXXX PACKAGE_NAME=" ${PACKAGE_NAME})
#message(STATUS "XXXXXXXXXXXXXXX CPACK_PACKAGE_FILE_NAME=" ${CPACK_PACKAGE_FILE_NAME})

set (CPACK_GENERATOR "RPM;TGZ;DEB")
set (CPACK_PACKAGING_INSTALL_PREFIX "/home/s/safe")

include (CPack)
include (CTest)
