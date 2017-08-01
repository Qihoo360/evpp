
set(ECHO_THRIFT echo.thrift)
execute_process(COMMAND ${THRIFT_COMPILER} --gen cpp ${ECHO_THRIFT}
                WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
execute_process(COMMAND ${THRIFT_COMPILER} --gen py ${ECHO_THRIFT}
                WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
set(ECHO_THRIFT_SRCS
    gen-cpp/echo_constants.cpp
    gen-cpp/echo_constants.h
    gen-cpp/echo_types.cpp
    gen-cpp/echo_types.h
    gen-cpp/Echo.cpp
    gen-cpp/Echo.h
    )

set(ECHO_SRCS
    echo_server.cc
    )

add_executable(evthrift_echo ${ECHO_SRCS} ${ECHO_THRIFT_SRCS})
target_link_libraries(evthrift_echo evthrift_static)


