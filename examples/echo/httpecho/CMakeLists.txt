file(GLOB example_SRCS *.cc)


include_directories(${PROJECT_SOURCE_DIR} ${PROJECT_SOURCE_DIR}/3rdparty)
link_directories(${PROJECT_SOURCE_DIR}/3rdparty/libevent-release-2.1.7-rc/build/lib/Debug/
				 ${PROJECT_SOURCE_DIR}/msvc/bin/Debug/
				${PROJECT_SOURCE_DIR}/build/lib/Debug/)
				 
add_executable(example_httpecho ${example_SRCS})

if (UNIX)
target_link_libraries(example_httpecho evpp event glog pthread)
else(UNIX)
#include_directories(${PROJECT_SOURCE_DIR} 
#					${PROJECT_SOURCE_DIR}/3rdparty/wininclude
#					${PROJECT_SOURCE_DIR}/3rdparty/glog-0.3.4/src/windows)
#
target_link_libraries(evpp_unittest evpp_static event libglog_static)
endif(UNIX)


