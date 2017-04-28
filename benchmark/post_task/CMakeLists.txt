
set(LINKED_LIBRARIES evpp_static ${DEPENDENT_LIBRARIES})
set(LINKED_LOCKFREE_LIBRARIES evpp_boost_static ${DEPENDENT_LIBRARIES})
set(LINKED_CONCURRENTQUEUE_LIBRARIES evpp_concurrentqueue_static ${DEPENDENT_LIBRARIES})
if (WIN32)
	link_directories(${PROJECT_SOURCE_DIR}/vsprojects/bin/${CMAKE_BUILD_TYPE}/
                     ${LIBRARY_OUTPUT_PATH}/${CMAKE_BUILD_TYPE}/
                     ${PROJECT_SOURCE_DIR}/3rdparty/glog-0.3.4/${CMAKE_BUILD_TYPE})
endif(WIN32)

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
	add_executable(benchmark_post_task_boost_lockfree_queue1 post_task1.cc)
	target_link_libraries(benchmark_post_task_boost_lockfree_queue1 ${LINKED_LOCKFREE_LIBRARIES})

	add_executable(benchmark_post_task_boost_lockfree_queue2 post_task2.cc)
	target_link_libraries(benchmark_post_task_boost_lockfree_queue2 ${LINKED_LOCKFREE_LIBRARIES})

	add_executable(benchmark_post_task_boost_lockfree_queue3 post_task3.cc)
	target_link_libraries(benchmark_post_task_boost_lockfree_queue3 ${LINKED_LOCKFREE_LIBRARIES})

	add_executable(benchmark_post_task_boost_lockfree_queue4 post_task4.cc)
	target_link_libraries(benchmark_post_task_boost_lockfree_queue4 ${LINKED_LOCKFREE_LIBRARIES})

	add_executable(benchmark_post_task_boost_lockfree_queue5 post_task5.cc)
	target_link_libraries(benchmark_post_task_boost_lockfree_queue5 ${LINKED_LOCKFREE_LIBRARIES})
	
	add_executable(benchmark_post_task_boost_lockfree_queue6 post_task6.cc)
	target_link_libraries(benchmark_post_task_boost_lockfree_queue6 ${LINKED_LOCKFREE_LIBRARIES})


    add_executable(benchmark_post_task_concurrentqueue1 post_task1.cc)
    target_link_libraries(benchmark_post_task_concurrentqueue1 ${LINKED_CONCURRENTQUEUE_LIBRARIES})

    add_executable(benchmark_post_task_concurrentqueue2 post_task2.cc)
    target_link_libraries(benchmark_post_task_concurrentqueue2 ${LINKED_CONCURRENTQUEUE_LIBRARIES})

    add_executable(benchmark_post_task_concurrentqueue3 post_task3.cc)
    target_link_libraries(benchmark_post_task_concurrentqueue3 ${LINKED_CONCURRENTQUEUE_LIBRARIES})

    add_executable(benchmark_post_task_concurrentqueue4 post_task4.cc)
    target_link_libraries(benchmark_post_task_concurrentqueue4 ${LINKED_CONCURRENTQUEUE_LIBRARIES})

    add_executable(benchmark_post_task_concurrentqueue5 post_task5.cc)
    target_link_libraries(benchmark_post_task_concurrentqueue5 ${LINKED_CONCURRENTQUEUE_LIBRARIES})

    add_executable(benchmark_post_task_concurrentqueue6 post_task6.cc)
    target_link_libraries(benchmark_post_task_concurrentqueue6 ${LINKED_CONCURRENTQUEUE_LIBRARIES})
endif (UNIX)
