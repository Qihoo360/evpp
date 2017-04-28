include_directories(${PROJECT_SOURCE_DIR}/apps ${PROJECT_SOURCE_DIR}/3rdparty)

if (UNIX)
set(LIBRARIES evpp ${DEPENDENT_LIBRARIES})
else(UNIX)
set(LIBRARIES evpp_static ${DEPENDENT_LIBRARIES})
endif(UNIX)

add_executable(example_evnsq_tail evnsq_tail/main.cc)
target_link_libraries(example_evnsq_tail evnsq_static ${LIBRARIES})

set (evnsq_producer_SRCS evnsq_producer/main.cc)
if (WIN32)
	list (APPEND evnsq_producer_SRCS ${PROJECT_SOURCE_DIR}/3rdparty/win32code/getopt/getopt.c)
    list (APPEND evnsq_producer_SRCS ${PROJECT_SOURCE_DIR}/3rdparty/win32code/getopt/getopt_long.c)
endif (WIN32)

add_executable(example_evnsq_producer ${evnsq_producer_SRCS})
target_link_libraries(example_evnsq_producer evnsq_static ${LIBRARIES})
