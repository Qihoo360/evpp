#pragma once

#include <string>
#include <vector>
#include <stdint.h>
#include <sstream>

namespace evpp
{
    class StringUtil
    {
    public:
        static std::vector<std::string> split(const std::string& src, const std::string& delim, size_t maxParts = size_t(-1));
        static uint32_t hash(const std::string& str);

        static std::string lower(const std::string& src);
        static std::string upper(const std::string& src);

        static std::string lstrip(const std::string& src);
        static std::string rstrip(const std::string& src);
        static std::string strip(const std::string& src);

        static std::string hex(const std::string& src);
        static std::string hex(const uint8_t* src, size_t srcLen);

        static std::string base64Encode(const std::string& src);
        static std::string base64Decode(const std::string& src);

        static std::string md5Bin(const std::string& src);
        static std::string md5Hex(const std::string& src);

        static std::string sha1Bin(const std::string& src);
        static std::string sha1Hex(const std::string& src);

        template<typename T>
        static std::string toString(const T& in)
        {
            std::stringstream str;
            str << in;
            return str.str();    
        }

        static std::string toString(const char* in)
        {
            return std::string(in);    
        }

        static std::string toString(const std::string& in)
        {
            return std::string(in);
        }
    };    
}

