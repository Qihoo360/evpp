include_directories(gen-cpp)
set(PING_THRIFT ping.thrift)
execute_process(COMMAND ${THRIFT_COMPILER} --gen cpp ${PING_THRIFT}
                WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
execute_process(COMMAND ${THRIFT_COMPILER} --gen py ${PING_THRIFT}
                WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
set(PING_THRIFT_SRCS
    gen-cpp/ping_constants.cpp
    gen-cpp/ping_types.cpp
    gen-cpp/Ping.cpp
    )
set(PING_SRCS
    ping_server.cc
    )
add_executable(evthrift_ping ${PING_SRCS} ${PING_THRIFT_SRCS})
target_link_libraries(evthrift_ping evthrift_static)
