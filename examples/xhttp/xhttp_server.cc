
#include <iostream>
#include <string>
#include <evpp/stringutil.h>

int main(int argc, char const *argv[])
{
    std::string s = "This is test";
    std::cout << s << std::endl;
    std::cout << evpp::StringUtil::upper(s) << std::endl;
    return 0;
}
