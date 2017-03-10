#!/bin/bash
g++ -O2 -finline-limit=1000 server.cpp -o server -lpthread -lboost_thread -lboost_system
g++ -O2 -finline-limit=1000 client.cpp -o client -lpthread -lboost_thread -lboost_system
# g++ -O2 -finline-limit=1000 -I../asio-1.4.5/include server.cpp -o server145 -lpthread
# g++ -O2 -finline-limit=1000 -I../asio-1.4.5/include client.cpp -o client145 -lpthread
