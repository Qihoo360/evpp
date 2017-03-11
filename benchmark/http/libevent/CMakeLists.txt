
#set (SRC libevent_http_bench.c)
#if (WIN32)
#    list (APPEND SRC ${PROJECT_SOURCE_DIR}/3rdparty/win32code/getopt/getopt.c)
#    list (APPEND SRC ${PROJECT_SOURCE_DIR}/3rdparty/win32code/getopt/getopt_long.c)
#	include_directories(${PROJECT_SOURCE_DIR}/3rdparty/wininclude)
#endif (WIN32)

add_executable(benchmark_libevent_http_bench ${SRC})