if (UNIX)
    add_subdirectory(asio)
    add_subdirectory(libevent)
endif(UNIX)

#add_executable(benchmark_tcp_evpp_echo evpp/main.cc evpp/codec.h)
#target_link_libraries(benchmark_tcp_evpp_echo ${EVPPLIBRARIES})
#
#add_executable(benchmark_tcp_evpp_echo_multi_accept evpp/multi_accept.cc evpp/codec.h)
#target_link_libraries(benchmark_tcp_evpp_echo_multi_accept ${EVPPLIBRARIES})
