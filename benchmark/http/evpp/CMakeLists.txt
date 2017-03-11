
if (UNIX)
else(UNIX)
link_directories(${PROJECT_SOURCE_DIR}/vsprojects/bin/${CMAKE_BUILD_TYPE}/
                 ${LIBRARY_OUTPUT_PATH}/${CMAKE_BUILD_TYPE}/
                 ${PROJECT_SOURCE_DIR}/3rdparty/glog-0.3.4/${CMAKE_BUILD_TYPE})
endif(UNIX)
				 
add_executable(benchmark_http_evpp evpp_http_bench.cc )

if (UNIX)
target_link_libraries(benchmark_http_evpp evpp_static event glog pthread)
else(UNIX)
#include_directories(${PROJECT_SOURCE_DIR} 
#					${PROJECT_SOURCE_DIR}/3rdparty/wininclude
#					${PROJECT_SOURCE_DIR}/3rdparty/glog-0.3.4/src/windows)
#
target_link_libraries(benchmark_http_evpp evpp_static event libglog_static)
endif(UNIX)


