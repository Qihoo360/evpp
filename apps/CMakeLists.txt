
include_directories(${PROJECT_SOURCE_DIR}/apps ${PROJECT_SOURCE_DIR}/3rdparty)

if (UNIX)
set(LIBRARIES evpp_concurrentqueue event glog pthread)
link_directories("/home/s/safe/lib" ${PROJECT_BUILD_DIR}/lib)
else(UNIX)
set(LIBRARIES evpp_static event glog)
link_directories(${PROJECT_SOURCE_DIR}/vsprojects/bin/${CMAKE_BUILD_TYPE}/
				 ${LIBRARY_OUTPUT_PATH}/${CMAKE_BUILD_TYPE}/)
endif(UNIX)

add_subdirectory(evnsq)
add_subdirectory(evmc)

add_subdirectory(evthrift)
#if(THRIFT_COMPILER AND THRIFT_INCLUDE_DIR AND THRIFT_LIBRARY)
#    add_subdirectory(evthrift)
#else()
#    add_subdirectory(evthrift EXCLUDE_FROM_ALL)
#endif()
#
