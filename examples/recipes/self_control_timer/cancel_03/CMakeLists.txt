file(GLOB SRCS *.cc *.h)

add_executable(example_recipes_self_control_timer_cancel_03 ${SRCS})
target_link_libraries(example_recipes_self_control_timer_cancel_03 event)



