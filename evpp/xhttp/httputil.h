#pragma once

#include <stdint.h>
#include <string>

namespace evpp
{
namespace xhttp {
    class HttpUtil
    {
    public:
        static const std::string& codeReason(int code);
        static const char* methodStr(uint8_t method);
    
        static std::string escape(const std::string& src);
        static std::string unescape(const std::string& src);
 
        //http header key is Http-Head-Case format
        static std::string normalizeHeader(const std::string& src);
    };
}
}

