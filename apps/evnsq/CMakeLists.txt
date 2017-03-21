
# evnsq
file(GLOB evnsq_SRCS *.cc evnsq/*.h)
file(GLOB evnsq_PUBLIC_HEADERS *.h)

add_library(evnsq SHARED ${evnsq_SRCS})
target_link_libraries(evnsq ${LIBRARIES})

add_library(evnsq_static STATIC ${evnsq_SRCS})
target_link_libraries(evnsq_static ${LIBRARIES})

#----------------------------------------
# install
install (
  TARGETS evnsq evnsq_static
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib)

install (FILES ${evnsq_PUBLIC_HEADERS} DESTINATION "include/evnsq")



#----------------------------------------
# unit tests
add_executable(evnsq_tail evnsqtail/main.cc)
target_link_libraries(evnsq_tail evnsq_static ${LIBRARIES})


set (unittest_evnsq_producer_with_auth_SRCS test/producer_with_auth/main.cc)
if (WIN32)
	list (APPEND unittest_evnsq_producer_with_auth_SRCS ${PROJECT_SOURCE_DIR}/3rdparty/win32code/getopt/getopt.c)
    list (APPEND unittest_evnsq_producer_with_auth_SRCS ${PROJECT_SOURCE_DIR}/3rdparty/win32code/getopt/getopt_long.c)
endif (WIN32)

add_executable(unittest_evnsq_producer_with_auth ${unittest_evnsq_producer_with_auth_SRCS})
target_link_libraries(unittest_evnsq_producer_with_auth evnsq_static ${LIBRARIES})

