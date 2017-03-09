include_directories(${PROJECT_SOURCE_DIR} ${PROJECT_SOURCE_DIR}/3rdparty)

if (UNIX)
set(EVPPLIBRARIES evpp event glog pthread)
else(UNIX)
set(EVPPLIBRARIES evpp_static event libglog_static)
link_directories(${PROJECT_SOURCE_DIR}/vsprojects/bin/${CMAKE_BUILD_TYPE}/
				 ${PROJECT_SOURCE_DIR}/3rdparty/glog-0.3.4/${CMAKE_BUILD_TYPE})
endif(UNIX)

#add_executable(benchmark_tcp_evpp_echo evpp/main.cc evpp/codec.h)
#target_link_libraries(benchmark_tcp_evpp_echo ${EVPPLIBRARIES})
#
#add_executable(benchmark_tcp_evpp_echo_multi_accept evpp/multi_accept.cc evpp/codec.h)
#target_link_libraries(benchmark_tcp_evpp_echo_multi_accept ${EVPPLIBRARIES})
