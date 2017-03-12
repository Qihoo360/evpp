
if (UNIX)
else(UNIX)
link_directories(${PROJECT_SOURCE_DIR}/vsprojects/bin/${CMAKE_BUILD_TYPE}/
                 ${LIBRARY_OUTPUT_PATH}/${CMAKE_BUILD_TYPE}/
                 ${PROJECT_SOURCE_DIR}/3rdparty/glog-0.3.4/${CMAKE_BUILD_TYPE})
endif(UNIX)

set (SRCS evpp_ioevent_bench.cc)
if (WIN32)
        list (APPEND SRCS ${PROJECT_SOURCE_DIR}/3rdparty/win32code/getopt/getopt.c)
    list (APPEND SRCS ${PROJECT_SOURCE_DIR}/3rdparty/win32code/getopt/getopt_long.c)
endif (WIN32)

add_executable(benchmark_ioevent_evpp ${SRCS})

if (UNIX)
target_link_libraries(benchmark_ioevent_evpp evpp_static event glog pthread)
else(UNIX)
#include_directories(${PROJECT_SOURCE_DIR} 
#					${PROJECT_SOURCE_DIR}/3rdparty/wininclude
#					${PROJECT_SOURCE_DIR}/3rdparty/glog-0.3.4/src/windows)
#
target_link_libraries(benchmark_ioevent_evpp evpp_static event libglog_static)
endif(UNIX)


