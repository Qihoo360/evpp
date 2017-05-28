
add_executable(evmc_test mcpool_test.cc)
target_link_libraries(evmc_test evmc_static ${LIBRARIES})
