include_directories(${PROJECT_SOURCE_DIR} ${PROJECT_SOURCE_DIR}/3rdparty)


if (UNIX)
set(LIBRARIES evpp_static event glog pthread)
else(UNIX)
set(LIBRARIES evpp_static event libglog_static)
link_directories(${PROJECT_SOURCE_DIR}/vsprojects/bin/${CMAKE_BUILD_TYPE}/
				 ${PROJECT_SOURCE_DIR}/3rdparty/glog-0.3.4/${CMAKE_BUILD_TYPE})
endif(UNIX)

add_subdirectory(apps)

add_executable(example_httpecho echo/httpecho/main.cc)
target_link_libraries(example_httpecho ${LIBRARIES})

add_executable(example_tcpecho echo/tcpecho/main.cc)
target_link_libraries(example_tcpecho ${LIBRARIES})

add_executable(example_udpecho echo/udpecho/main.cc)
target_link_libraries(example_udpecho ${LIBRARIES})

add_executable(example_pingpong_client pingpong/client.cc)
target_link_libraries(example_pingpong_client ${LIBRARIES})

add_executable(example_pingpong_server pingpong/server.cc)
target_link_libraries(example_pingpong_server ${LIBRARIES})

add_executable(example_tcp_client tcp/tcp_client.cc)
target_link_libraries(example_tcp_client ${LIBRARIES})

add_executable(example_tcp_server tcp/tcp_server.cc)
target_link_libraries(example_tcp_server ${LIBRARIES})

add_executable(example_multi_accept multi_accept/main.cc)
target_link_libraries(example_multi_accept ${LIBRARIES})

add_executable(example_twisted_pubsubserver twisted/pubsubserver/main.cc)
target_link_libraries(example_twisted_pubsubserver ${LIBRARIES})

add_executable(example_twisted_webserver twisted/webserver/main.cc)
target_link_libraries(example_twisted_webserver ${LIBRARIES})

add_executable(example_timer01 timer/timer01.cc)
target_link_libraries(example_timer01 ${LIBRARIES})

add_executable(example_timer02 timer/timer02.cc)
target_link_libraries(example_timer02 ${LIBRARIES})

add_executable(example_timer03 timer/timer03.cc)
target_link_libraries(example_timer03 ${LIBRARIES})

add_executable(example_timer04 timer/timer04.cc)
target_link_libraries(example_timer04 ${LIBRARIES})

add_executable(example_timer05 timer/timer05.cc)
target_link_libraries(example_timer05 ${LIBRARIES})

add_executable(example_http_client_request01 http_client_request/client01.cc)
target_link_libraries(example_http_client_request01 ${LIBRARIES})

add_executable(example_http_client_request02 http_client_request/client02.cc)
target_link_libraries(example_http_client_request02 ${LIBRARIES})

add_executable(example_http_client_request03 http_client_request/client03.cc)
target_link_libraries(example_http_client_request03 ${LIBRARIES})

add_executable(example_http_client_request04 http_client_request/client04.cc)
target_link_libraries(example_http_client_request04 ${LIBRARIES})

add_executable(example_http_client_request05 http_client_request/client05.cc)
target_link_libraries(example_http_client_request05 ${LIBRARIES})
