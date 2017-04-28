
include_directories(${PROJECT_SOURCE_DIR}/apps ${PROJECT_SOURCE_DIR}/3rdparty)

if (UNIX)
set(LIBRARIES evpp_concurrentqueue event glog pthread)
link_directories("/home/s/safe/lib" ${PROJECT_BUILD_DIR}/lib)
else(UNIX)
set(LIBRARIES evpp_static event glog)
link_directories(${PROJECT_SOURCE_DIR}/vsprojects/bin/${CMAKE_BUILD_TYPE}/
				 ${LIBRARY_OUTPUT_PATH}/${CMAKE_BUILD_TYPE}/
				 ${PROJECT_SOURCE_DIR}/3rdparty/glog-0.3.4/${CMAKE_BUILD_TYPE})
endif(UNIX)

add_subdirectory(evnsq)

#----------------------------------------
# evmc
set(CMAKE_C_FLAGS -std=gnu99)
file(GLOB evmc_SRCS evmc/*.cc
                    evmc/*.h
                    ${PROJECT_SOURCE_DIR}/3rdparty/libhashkit/*.c)
file(GLOB evmc_PUBLIC_HEADERS evmc/*.h)

add_library(evmc SHARED ${evmc_SRCS})
target_link_libraries(evmc ${LIBRARIES})

add_library(evmc_static STATIC ${evmc_SRCS})
target_link_libraries(evmc_static ${LIBRARIES})

add_executable(evmc_test evmc/test/mcpool_test.cc)
target_link_libraries(evmc_test evmc_static ${LIBRARIES})

set (CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")
include (utils)
include (packages)
set_target_properties (
    evmc evnsq PROPERTIES
    VERSION     "${PACKAGE_VERSION}"
    SOVERSION   "${PACKAGE_SOVERSION}"
)

#----------------------------------------
# install
install (
  TARGETS evmc evmc_static
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib)

install (FILES ${evmc_PUBLIC_HEADERS} DESTINATION "include/evmc")
