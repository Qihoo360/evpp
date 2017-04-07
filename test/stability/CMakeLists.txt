#include_directories(${PROJECT_SOURCE_DIR} ${PROJECT_SOURCE_DIR}/3rdparty/gtest ${PROJECT_SOURCE_DIR}/3rdparty)
#
#if (MSVC)
#link_directories(${PROJECT_SOURCE_DIR}/vsprojects/bin/${CMAKE_BUILD_TYPE}/
#				 ${LIBRARY_OUTPUT_PATH}/${CMAKE_BUILD_TYPE}/
#				 ${PROJECT_SOURCE_DIR}/3rdparty/glog-0.3.4/${CMAKE_BUILD_TYPE})
#endif (MSVC)

add_executable(test_evpp_stability stability.cc)

if (UNIX)
    target_link_libraries(test_evpp_stability evpp event glog pthread)

    add_executable(test_evpp_stability_boost_lockfree stability.cc)
    target_link_libraries(test_evpp_stability_boost_lockfree evpp_boost event glog pthread)

    add_executable(test_evpp_stability_concurrentqueue stability.cc)
    target_link_libraries(test_evpp_stability_concurrentqueue evpp_concurrentqueue event glog pthread)

else (UNIX)
target_link_libraries(test_evpp_stability evpp_static event libglog_static)
endif (UNIX)


