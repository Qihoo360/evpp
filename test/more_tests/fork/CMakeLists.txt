file(GLOB SROUCES *.cc *.h)

if (UNIX)
    add_executable(test_evpp_after_fork ${SROUCES})
    target_link_libraries(test_evpp_after_fork evpp_static ${DEPENDENT_LIBRARIES})
endif (UNIX)

