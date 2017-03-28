
if (UNIX)
	set(LINKED_LIBRARIES evpp_static event glog pthread)
	set(LINKED_LOCKFREE_LIBRARIES evpp_boost_static event glog pthread)
else(UNIX)
	set(LINKED_LIBRARIES evpp_static event libglog_static)
	link_directories(${PROJECT_SOURCE_DIR}/vsprojects/bin/${CMAKE_BUILD_TYPE}/
                     ${LIBRARY_OUTPUT_PATH}/${CMAKE_BUILD_TYPE}/
                     ${PROJECT_SOURCE_DIR}/3rdparty/glog-0.3.4/${CMAKE_BUILD_TYPE})
endif(UNIX)

add_executable(benchmark_post_task1 post_task1.cc)
target_link_libraries(benchmark_post_task1 ${LINKED_LIBRARIES})

add_executable(benchmark_post_task2 post_task2.cc)
target_link_libraries(benchmark_post_task2 ${LINKED_LIBRARIES})

add_executable(benchmark_post_task3 post_task3.cc)
target_link_libraries(benchmark_post_task3 ${LINKED_LIBRARIES})

add_executable(benchmark_post_task4 post_task4.cc)
target_link_libraries(benchmark_post_task4 ${LINKED_LIBRARIES})

add_executable(benchmark_post_task5 post_task5.cc)
target_link_libraries(benchmark_post_task5 ${LINKED_LIBRARIES})

add_executable(benchmark_post_task6 post_task6.cc)
target_link_libraries(benchmark_post_task6 ${LINKED_LIBRARIES})

if (UNIX)
	add_executable(benchmark_post_lockfree_task1 post_task1.cc)
	target_link_libraries(benchmark_post_lockfree_task1 ${LINKED_LOCKFREE_LIBRARIES})

	add_executable(benchmark_post_lockfree_task2 post_task2.cc)
	target_link_libraries(benchmark_post_lockfree_task2 ${LINKED_LOCKFREE_LIBRARIES})

	add_executable(benchmark_post_lockfree_task3 post_task3.cc)
	target_link_libraries(benchmark_post_lockfree_task3 ${LINKED_LOCKFREE_LIBRARIES})

	add_executable(benchmark_post_lockfree_task4 post_task4.cc)
	target_link_libraries(benchmark_post_lockfree_task4 ${LINKED_LOCKFREE_LIBRARIES})

	add_executable(benchmark_post_lockfree_task5 post_task5.cc)
	target_link_libraries(benchmark_post_lockfree_task5 ${LINKED_LOCKFREE_LIBRARIES})
	
	add_executable(benchmark_post_lockfree_task6 post_task6.cc)
	target_link_libraries(benchmark_post_lockfree_task6 ${LINKED_LOCKFREE_LIBRARIES})
endif (UNIX)