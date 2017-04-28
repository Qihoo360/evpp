include_directories(${PROJECT_SOURCE_DIR} ${PROJECT_SOURCE_DIR}/3rdparty)

set(LIBRARIES evpp_static ${DEPENDENT_LIBRARIES})
if (WIN32)
link_directories(${PROJECT_SOURCE_DIR}/vsprojects/bin/${CMAKE_BUILD_TYPE}/
				 ${PROJECT_SOURCE_DIR}/3rdparty/glog-0.3.4/${CMAKE_BUILD_TYPE})
endif(WIN32)

add_executable(benchmark_pingpong_header_body_client client.cc)
target_link_libraries(benchmark_pingpong_header_body_client ${LIBRARIES})

add_executable(benchmark_pingpong_header_body_server server.cc)
target_link_libraries(benchmark_pingpong_header_body_server ${LIBRARIES})
