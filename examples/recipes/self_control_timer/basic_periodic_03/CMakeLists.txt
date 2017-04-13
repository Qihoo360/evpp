#include_directories(${PROJECT_SOURCE_DIR} ${PROJECT_SOURCE_DIR}/3rdparty)
#
#
#if (UNIX)
#set(LIBRARIES evpp_static event glog pthread)
#else(UNIX)
#set(LIBRARIES evpp_static event libglog_static)
#link_directories(${PROJECT_SOURCE_DIR}/vsprojects/bin/${CMAKE_BUILD_TYPE}/
#				 ${PROJECT_SOURCE_DIR}/3rdparty/glog-0.3.4/${CMAKE_BUILD_TYPE})
#endif(UNIX)
#

file(GLOB SRCS *.cc *.h)

add_executable(example_recipes_self_control_timer_basic_periodic_03 ${SRCS})
target_link_libraries(example_recipes_self_control_timer_basic_periodic_03 event)



