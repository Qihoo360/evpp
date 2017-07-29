if (WIN32)
    # debug
    set(THRIFT_LIBRARY thriftmdd ssleay32 libeay32)
else ()
    set(THRIFT_LIBRARY thrift)
endif ()

set(EVPP_THRIFT_SRCS
    thrift_conn.cc thrift_conn.h
    thrift_server.cc thrift_server.h
    )    

add_library(evthrift_static ${EVPP_THRIFT_SRCS})
target_link_libraries(evthrift_static ${LIBRARIES} ${THRIFT_LIBRARY})

add_subdirectory(tests)

