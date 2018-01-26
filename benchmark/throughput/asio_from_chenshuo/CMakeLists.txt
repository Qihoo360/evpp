find_package(Boost REQUIRED COMPONENTS system thread)

add_executable(benchmark_tcp_asio_client client.cpp)
target_link_libraries(benchmark_tcp_asio_client ${Boost_SYSTEM_LIBRARY} ${Boost_THREAD_LIBRARY} pthread)

add_executable(benchmark_tcp_asio_server server.cpp)
target_link_libraries(benchmark_tcp_asio_server ${Boost_SYSTEM_LIBRARY} ${Boost_THREAD_LIBRARY} pthread)

