
file(GLOB evpp_SRCS *.cc */*.cc *.h */*.h)
file(GLOB evpp_PUBLIC_HEADERS *.h)
file(GLOB evpp_HTTP_PUBLIC_HEADERS http/*.h)
file(GLOB evpp_UDP_PUBLIC_HEADERS udp/*.h)

message(STATUS "evpp_SRCS : " ${evpp_SRCS})

set (MACOSX_RPATH ${LIBRARY_OUTPUT_PATH})

include_directories(${PROJECT_SOURCE_DIR})

add_library(evpp_static STATIC ${evpp_SRCS})

if (UNIX)
    add_library(evpp SHARED ${evpp_SRCS})
    target_link_libraries(evpp        event glog pthread)
    target_link_libraries(evpp_static event glog pthread)

    add_library(evpp_boost SHARED ${evpp_SRCS})
    add_library(evpp_boost_static STATIC ${evpp_SRCS})
    target_compile_definitions(evpp_boost PRIVATE -DH_HAVE_BOOST=1)
    target_compile_definitions(evpp_boost_static PRIVATE -DH_HAVE_BOOST=1)
    target_link_libraries(evpp_boost        event glog pthread)
    target_link_libraries(evpp_boost_static event glog pthread)

    set (CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")
    include (utils)
    include (packages)
    set_target_properties (
        evpp evpp_boost PROPERTIES
        VERSION     "${PACKAGE_VERSION}"
        SOVERSION   "${PACKAGE_SOVERSION}"
    )

    install (
      TARGETS evpp evpp_static evpp_boost evpp_boost_static
      EXPORT ${PACKAGE_NAME}
      RUNTIME DESTINATION bin
      LIBRARY DESTINATION lib
      ARCHIVE DESTINATION lib)

    install (FILES ${evpp_PUBLIC_HEADERS} DESTINATION "include/evpp")
    install (FILES ${evpp_HTTP_PUBLIC_HEADERS} DESTINATION "include/evpp/http")
    install (FILES ${evpp_UDP_PUBLIC_HEADERS} DESTINATION "include/evpp/udp")

else (UNIX)
	link_directories(${PROJECT_SOURCE_DIR}/vsprojects/bin/${CMAKE_BUILD_TYPE})
    target_link_libraries(evpp_static event libglog_static)
endif (UNIX)

