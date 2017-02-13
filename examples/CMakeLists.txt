include_directories(${PROJECT_SOURCE_DIR} ${PROJECT_SOURCE_DIR}/3rdparty)


if (UNIX)
set(LIBRARIES evpp event glog pthread)
else(UNIX)
set(LIBRARIES evpp_static event libglog_static)
link_directories(${PROJECT_SOURCE_DIR}/3rdparty/libevent-release-2.1.7-rc/build/lib/Debug/
				 ${PROJECT_SOURCE_DIR}/build/lib/Debug/)
endif(UNIX)

add_executable(example_httpecho echo/httpecho/main.cc)
target_link_libraries(example_httpecho ${LIBRARIES})

add_executable(example_tcpecho echo/tcpecho/main.cc)
target_link_libraries(example_tcpecho ${LIBRARIES})

add_executable(example_udpecho echo/udpecho/main.cc)
target_link_libraries(example_udpecho ${LIBRARIES})

add_executable(example_pingpong_client pingpong/client/main.cc)
target_link_libraries(example_pingpong_client ${LIBRARIES})

add_executable(example_pingpong_server pingpong/server/main.cc)
target_link_libraries(example_pingpong_server ${LIBRARIES})
