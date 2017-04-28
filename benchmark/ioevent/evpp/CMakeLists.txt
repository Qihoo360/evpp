
if (UNIX)
else (UNIX)
link_directories(${PROJECT_SOURCE_DIR}/vsprojects/bin/${CMAKE_BUILD_TYPE}/
                 ${LIBRARY_OUTPUT_PATH}/${CMAKE_BUILD_TYPE}/
                 ${PROJECT_SOURCE_DIR}/3rdparty/glog-0.3.4/${CMAKE_BUILD_TYPE})
endif(UNIX)

set (evpp_SRCS evpp_ioevent_bench.cc)
if (WIN32)
    list (APPEND evpp_SRCS ${PROJECT_SOURCE_DIR}/3rdparty/win32code/getopt/getopt.c)
    list (APPEND evpp_SRCS ${PROJECT_SOURCE_DIR}/3rdparty/win32code/getopt/getopt_long.c)
endif (WIN32)
add_executable(benchmark_ioevent_evpp ${evpp_SRCS})
target_link_libraries(benchmark_ioevent_evpp evpp_static ${DEPENDENT_LIBRARIES})



set (evpp_SRCS evpp_ioevent_pipe_watcher.cc)
if (WIN32)
    list (APPEND evpp_SRCS ${PROJECT_SOURCE_DIR}/3rdparty/win32code/getopt/getopt.c)
    list (APPEND evpp_SRCS ${PROJECT_SOURCE_DIR}/3rdparty/win32code/getopt/getopt_long.c)
endif (WIN32)

add_executable(benchmark_ioevent_evpp_pipe_watcher ${evpp_SRCS})
target_link_libraries(benchmark_ioevent_evpp_pipe_watcher evpp_static ${DEPENDENT_LIBRARIES})


