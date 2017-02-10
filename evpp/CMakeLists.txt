
set(evpp_DEPENDENT_LIBRARY event glog pthread)

file(GLOB evpp_SRCS *.cc */*.cc *.h */*.h)

set(MYINCLUDEDIR ${PROJECT_SOURCE_DIR})
message(STATUS "evpp_SRCS : " ${evpp_SRCS})
message(STATUS "This is MYINCLUDEDIR : " ${MYINCLUDEDIR})

include_directories(${MYINCLUDEDIR})
add_library(evpp SHARED ${evpp_SRCS})
add_library(evpp_static STATIC ${evpp_SRCS})
set_target_properties(evpp_static PROPERTIES OUTPUT_NAME "evpp")
target_link_libraries(evpp ${evpp_DEPENDENT_LIBRARY})
target_link_libraries(evpp_static ${evpp_DEPENDENT_LIBRARY})
