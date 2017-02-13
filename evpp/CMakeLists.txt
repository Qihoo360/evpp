
file(GLOB evpp_SRCS *.cc */*.cc *.h */*.h)
file(GLOB evpp_PUBLIC_HEADERS *.h)
file(GLOB evpp_HTTP_PUBLIC_HEADERS http/*.h)
file(GLOB evpp_UDP_PUBLIC_HEADERS udp/*.h)

message(STATUS "evpp_SRCS : " ${evpp_SRCS})

include_directories(${PROJECT_SOURCE_DIR})

add_library(evpp SHARED ${evpp_SRCS})
add_library(evpp_static STATIC ${evpp_SRCS})

if (UNIX)
target_link_libraries(evpp        event glog pthread)
target_link_libraries(evpp_static event glog pthread)
else (UNIX)
target_link_libraries(evpp        event libglog_static)
target_link_libraries(evpp_static event libglog_static)
endif (UNIX)


install (
  TARGETS evpp evpp_static
  EXPORT ${PACKAGE_NAME}
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib)

#install (FILES listener.h tcp_conn.h DESTINATION "include/evpp")

install (FILES ${evpp_PUBLIC_HEADERS} DESTINATION "include/evpp")
install (FILES ${evpp_HTTP_PUBLIC_HEADERS} DESTINATION "include/evpp/http")
install (FILES ${evpp_UDP_PUBLIC_HEADERS} DESTINATION "include/evpp/udp")

