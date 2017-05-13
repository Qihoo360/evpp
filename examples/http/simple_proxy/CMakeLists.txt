file(GLOB example_SRCS *.cc)


#include_directories(${PROJECT_SOURCE_DIR} ${PROJECT_SOURCE_DIR}/3rdparty)
#link_directories(${PROJECT_SOURCE_DIR}/3rdparty/libevent-release-2.1.7-rc/build/lib/Debug/
#				 ${PROJECT_SOURCE_DIR}/msvc/bin/Debug/
#				${PROJECT_SOURCE_DIR}/build/lib/Debug/)
#				 

add_executable(example_http_simple_proxy ${example_SRCS})
target_link_libraries(example_http_simple_proxy ${LIBRARIES})


