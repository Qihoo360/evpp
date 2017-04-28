include_directories(${PROJECT_SOURCE_DIR} ${PROJECT_SOURCE_DIR}/3rdparty)

set(LIBRARIES evpp_static ${DEPENDENT_LIBRARIES})
if (WIN32)
link_directories(${PROJECT_SOURCE_DIR}/vsprojects/bin/${CMAKE_BUILD_TYPE}/
				 ${PROJECT_SOURCE_DIR}/3rdparty/glog-0.3.4/${CMAKE_BUILD_TYPE})
endif(WIN32)

add_executable(benchmark_pingpong_client client.cc)
target_link_libraries(benchmark_pingpong_client ${LIBRARIES})

add_executable(benchmark_pingpong_client_fixed_size client_fixed_size.cc)
target_link_libraries(benchmark_pingpong_client_fixed_size ${LIBRARIES})

add_executable(benchmark_pingpong_server server.cc)
target_link_libraries(benchmark_pingpong_server ${LIBRARIES})
