file(GLOB evpp_unittest_SRCS *.cc
							 ${PROJECT_SOURCE_DIR}/3rdparty/gtest/src/gtest-all.cc
							 ${PROJECT_SOURCE_DIR}/3rdparty/gtest/src/gtest_main.cc)


include_directories(${PROJECT_SOURCE_DIR} ${PROJECT_SOURCE_DIR}/3rdparty/gtest ${PROJECT_SOURCE_DIR}/3rdparty)

if (MSVC)
link_directories(${PROJECT_SOURCE_DIR}/3rdparty/libevent-release-2.1.7-rc/build/lib/${CMAKE_BUILD_TYPE}/
				 ${LIBRARY_OUTPUT_PATH}/${CMAKE_BUILD_TYPE}/
				 ${PROJECT_SOURCE_DIR}/3rdparty/glog-0.3.4/${CMAKE_BUILD_TYPE})
endif (MSVC)

add_executable(evpp_unittest ${evpp_unittest_SRCS})

if (UNIX)
target_link_libraries(evpp_unittest evpp event glog pthread)
else (UNIX)
target_link_libraries(evpp_unittest evpp_static event libglog_static)
endif (UNIX)

include (CTest)
add_test(NAME evpp_unittest COMMAND evpp_unittest)
