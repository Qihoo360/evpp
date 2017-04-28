file(GLOB SRCS *.cc *.h)

add_executable(example_recipes_self_control_timer_basic_01 ${SRCS})
target_link_libraries(example_recipes_self_control_timer_basic_01 event)




