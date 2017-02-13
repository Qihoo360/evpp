file(GLOB evpp_unittest_SRCS *.cc
							 ${PROJECT_SOURCE_DIR}/3rdparty/gtest/src/gtest-all.cc
							 ${PROJECT_SOURCE_DIR}/3rdparty/gtest/src/gtest_main.cc)


include_directories(${PROJECT_SOURCE_DIR} ${PROJECT_SOURCE_DIR}/3rdparty/gtest ${PROJECT_SOURCE_DIR}/3rdparty)
				 
add_executable(evpp_unittest ${evpp_unittest_SRCS})

if (UNIX)
target_link_libraries(evpp_unittest evpp event glog pthread)
else(UNIX)
link_directories(${PROJECT_SOURCE_DIR}/3rdparty/libevent-release-2.1.7-rc/build/lib/Debug/
				 ${PROJECT_SOURCE_DIR}/build/lib/Debug/)
target_link_libraries(evpp_unittest evpp_static event libglog_static)
endif(UNIX)

