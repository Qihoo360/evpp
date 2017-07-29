set(EVPP_THRIFT_SRCS
    thrift_conn.cc thrift_conn.h
    thrift_server.cc thrift_server.h
    )
add_library(evthrift_static ${EVPP_THRIFT_SRCS})
target_link_libraries(evthrift_static ${LIBRARIES} thrift)

#if(THRIFT_COMPILER AND THRIFT_INCLUDE_DIR AND THRIFT_LIBRARY)
#  include_directories(${CMAKE_CURRENT_SOURCE_DIR})
#  add_subdirectory(tests)
#endif()
