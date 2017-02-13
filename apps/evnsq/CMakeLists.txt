
file(GLOB evnsq_SRCS *.cc)

message(STATUS "evnsq_SRCS : " ${evnsq_SRCS})

if (UNIX)
set(LIBRARIES evpp_static event glog pthread)
include_directories(${PROJECT_SOURCE_DIR}/apps)
else(UNIX)
set(LIBRARIES evpp_static event libglog_static)
link_directories(${PROJECT_SOURCE_DIR}/3rdparty/libevent-release-2.1.7-rc/build/lib/${CMAKE_BUILD_TYPE}/
				 ${PROJECT_SOURCE_DIR}/build/lib/${CMAKE_BUILD_TYPE}/
				 ${PROJECT_SOURCE_DIR}/3rdparty/glog-0.3.4/${CMAKE_BUILD_TYPE})
endif(UNIX)


add_library(evnsq SHARED ${evnsq_SRCS})
target_link_libraries(evnsq ${LIBRARIES})

add_library(evnsq_static STATIC ${evnsq_SRCS})
target_link_libraries(evnsq_static ${LIBRARIES})

add_executable(evnsq_test test/main.cc)
target_link_libraries(evnsq_test evnsq_static ${LIBRARIES})

add_executable(evnsq_tail evnsqtail/main.cc)
target_link_libraries(evnsq_tail evnsq_static ${LIBRARIES})
