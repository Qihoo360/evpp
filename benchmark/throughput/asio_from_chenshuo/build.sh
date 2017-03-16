#!/bin/bash
g++ -O2 -finline-limit=1000 server.cpp -o benchmark_tcp_asio_server -lpthread -lboost_thread -lboost_system
g++ -O2 -finline-limit=1000 client.cpp -o benchmark_tcp_asio_client -lpthread -lboost_thread -lboost_system
