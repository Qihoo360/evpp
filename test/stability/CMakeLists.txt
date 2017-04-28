#include_directories(${PROJECT_SOURCE_DIR} ${PROJECT_SOURCE_DIR}/3rdparty/gtest ${PROJECT_SOURCE_DIR}/3rdparty)
#
#if (MSVC)
#link_directories(${PROJECT_SOURCE_DIR}/vsprojects/bin/${CMAKE_BUILD_TYPE}/
#				 ${LIBRARY_OUTPUT_PATH}/${CMAKE_BUILD_TYPE}/
#				 ${PROJECT_SOURCE_DIR}/3rdparty/glog-0.3.4/${CMAKE_BUILD_TYPE})
#endif (MSVC)

file(GLOB SROUCES *.cc *.h)
add_executable(test_evpp_stability ${SROUCES})
target_link_libraries(test_evpp_stability evpp_static ${DEPENDENT_LIBRARIES})

if (UNIX)
    add_executable(test_evpp_stability_boost_lockfree ${SROUCES})
    target_link_libraries(test_evpp_stability_boost_lockfree evpp_boost_static ${DEPENDENT_LIBRARIES})

    add_executable(test_evpp_stability_concurrentqueue ${SROUCES})
    target_link_libraries(test_evpp_stability_concurrentqueue evpp_concurrentqueue_static ${DEPENDENT_LIBRARIES})
endif (UNIX)

