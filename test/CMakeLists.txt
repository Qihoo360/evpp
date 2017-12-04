file(GLOB evpp_unittest_SRCS *.cc
							 ${PROJECT_SOURCE_DIR}/3rdparty/gtest/src/gtest-all.cc
							 ${PROJECT_SOURCE_DIR}/3rdparty/gtest/src/gtest_main.cc)
list(REMOVE_ITEM evpp_unittest_SRCS ${PROJECT_SOURCE_DIR}/test/https_client_test.cc)
file(GLOB evpp_https_unittest_SRCS https_client_test.cc
							 ${PROJECT_SOURCE_DIR}/3rdparty/gtest/src/gtest-all.cc
							 ${PROJECT_SOURCE_DIR}/3rdparty/gtest/src/gtest_main.cc)
include_directories(${PROJECT_SOURCE_DIR}/3rdparty ${PROJECT_SOURCE_DIR}/3rdparty/gtest)

if (MSVC)
link_directories(${LIBRARY_OUTPUT_PATH}/${CMAKE_BUILD_TYPE}/)
endif (MSVC)

add_executable(evpp_unittest ${evpp_unittest_SRCS})
target_link_libraries(evpp_unittest evpp_static ${DEPENDENT_LIBRARIES})

if (HTTPS)
add_executable(evpp_https_unittest ${evpp_https_unittest_SRCS})
target_link_libraries(evpp_https_unittest evpp_https_static ${DEPENDENT_LIBRARIES})
endif (HTTPS)

if (UNIX)
    add_executable(evpp_unittest_boost_lockfree ${evpp_unittest_SRCS})
    target_link_libraries(evpp_unittest_boost_lockfree evpp_boost ${DEPENDENT_LIBRARIES})

    add_executable(evpp_unittest_concurrentqueue ${evpp_unittest_SRCS})
    target_link_libraries(evpp_unittest_concurrentqueue evpp_concurrentqueue ${DEPENDENT_LIBRARIES})
endif (UNIX)

include (CTest)
add_test(NAME evpp_unittest COMMAND evpp_unittest)

if (UNIX)
    add_test(NAME evpp_unittest_concurrentqueue COMMAND evpp_unittest_concurrentqueue)
    add_test(NAME evpp_unittest_boost_lockfree  COMMAND evpp_unittest_boost_lockfree)
endif (UNIX)

add_subdirectory (stability)
add_subdirectory (more_tests)
