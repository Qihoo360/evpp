set(evpp_unittest_SRCS
            any_test.cc
            buffer_test.cc
            dns_resolver_test.cc
            event_condition_test.cc
            event_loop_test.cc
            event_loop_thread_pool_test.cc
            event_loop_thread_test.cc
            event_watcher_test.cc
            evhttp_client_test.cc
            glog_test.cc
            http_server_test.cc
            http_server_test2.cc
            invoke_timer_test.cc
            tcp_client_reconnect_test.cc
            tcp_server_test.cc
            time_test.cc
            udp_server_test.cc
            url_parser_test.cc
            utility_test.cc
            winmain.cc
            ${PROJECT_SOURCE_DIR}/3rdparty/gtest/src/gtest-all.cc
            ${PROJECT_SOURCE_DIR}/3rdparty/gtest/src/gtest_main.cc
)

set(evpp_unittest_DEPENDENT_LIBRARY evpp event glog pthread)

include_directories(${PROJECT_SOURCE_DIR} ${PROJECT_SOURCE_DIR}/3rdparty/gtest ${PROJECT_SOURCE_DIR}/3rdparty)
add_executable(evpp_unittest ${evpp_unittest_SRCS})
target_link_libraries(evpp_unittest evpp)
target_link_libraries(evpp_unittest ${evpp_unittest_DEPENDENT_LIBRARY})
