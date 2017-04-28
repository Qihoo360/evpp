file(GLOB SRCS *.cc *.h)

add_executable(example_recipes_self_control_timer_periodic_04 ${SRCS})
target_link_libraries(example_recipes_self_control_timer_periodic_04 event)



