
if (UNIX)
set(LIBRARIES evpp_static event glog pthread)
include_directories(${PROJECT_SOURCE_DIR}/apps)
else(UNIX)
set(LIBRARIES evpp_static event libglog_static)
link_directories(${PROJECT_SOURCE_DIR}/3rdparty/libevent-release-2.1.7-rc/build/lib/${CMAKE_BUILD_TYPE}/
				 ${PROJECT_SOURCE_DIR}/build/lib/${CMAKE_BUILD_TYPE}/
				 ${PROJECT_SOURCE_DIR}/3rdparty/glog-0.3.4/${CMAKE_BUILD_TYPE})
endif(UNIX)

#----------------------------------------
# evnsq
file(GLOB evnsq_SRCS evnsq/*.cc evnsq/*.h)
message(STATUS "evnsq_SRCS : " ${evnsq_SRCS})

add_library(evnsq SHARED ${evnsq_SRCS})
target_link_libraries(evnsq ${LIBRARIES})

add_library(evnsq_static STATIC ${evnsq_SRCS})
target_link_libraries(evnsq_static ${LIBRARIES})

add_executable(evnsq_test evnsq/test/main.cc)
target_link_libraries(evnsq_test evnsq_static ${LIBRARIES})

add_executable(evnsq_tail evnsq/evnsqtail/main.cc)
target_link_libraries(evnsq_tail evnsq_static ${LIBRARIES})

#----------------------------------------
# evmc
set(CMAKE_C_FLAGS -std=gnu99)
file(GLOB evmc_SRCS evmc/*.cc evmc/*.h ${PROJECT_SOURCE_DIR}/3rdparty/libhashkit/*.c)
message(STATUS "evmc_SRCS : " ${evmc_SRCS})

add_library(evmc SHARED ${evmc_SRCS})
target_link_libraries(evmc ${LIBRARIES})

add_library(evmc_static STATIC ${evmc_SRCS})
target_link_libraries(evmc_static ${LIBRARIES})

add_executable(evmc_test evmc/test/mcpool_test.cc)
target_link_libraries(evmc_test evmc_static ${LIBRARIES})

