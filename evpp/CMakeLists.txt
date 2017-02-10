set(evpp_SRCS
            buffer.cc
            connector.cc
            dns_resolver.cc
            duration.cc
            event_loop.cc
            event_loop_thread.cc
            event_loop_thread_pool.cc
            fd_channel.cc
            inner_pre.cc
            invoke_timer.cc
            libevent_compact.cc
            libevent_watcher.cc
            listener.cc
            pch.cc
            sockets.cc
            tcp_client.cc
            tcp_conn.cc
            tcp_server.cc
            udp/sync_udp_client.cc
            udp/udp_server.cc
            http/context.cc
            http/http_server.cc
            http/service.cc
            httpc/conn.cc
            httpc/conn_pool.cc
            httpc/request.cc
            httpc/response.cc
            httpc/url_parser.cc 
   )

set(evpp_DEPENDENT_LIBRARY event glog pthread)


SET(MYINCLUDEDIR ${PROJECT_SOURCE_DIR})
message(STATUS "This is MYINCLUDEDIR : " ${MYINCLUDEDIR})
include_directories(${MYINCLUDEDIR})
add_library(evpp SHARED ${evpp_SRCS})
add_library(evpp_static STATIC ${evpp_SRCS})
set_target_properties(evpp_static PROPERTIES OUTPUT_NAME "evpp")
target_link_libraries(evpp ${evpp_DEPENDENT_LIBRARY})
target_link_libraries(evpp_static ${evpp_DEPENDENT_LIBRARY})
