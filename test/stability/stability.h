#include <iostream>

#include <vector>
#include <string>

#include <stdio.h>
#include <stdlib.h>


static std::vector<int> g_listening_port;

std::string GetListenAddr() {
    return std::string("127.0.0.1:") + std::to_string(g_listening_port[0]);
}

