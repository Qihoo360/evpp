#include "stringutil.h"

#include <stdint.h>
#include <algorithm>
#include <functional>

extern "C"
{
#include "polarssl/base64.h"
#include "polarssl/md5.h"
#include "polarssl/sha1.h"    
}

using namespace std;

namespace evpp
{

    vector<string> StringUtil::split(const string& src, const string& delim, size_t maxParts)
    {
        if(maxParts == 0)
        {
            maxParts = size_t(-1);    
        }
        size_t lastPos = 0;
        size_t pos = 0;
        size_t size = src.size();

        vector<string> tokens;

        while(pos < size)
        {
            pos = lastPos;
            while(pos < size && delim.find_first_of(src[pos]) == string::npos)
            {
                ++pos;    
            }    

            if(pos - lastPos > 0)
            {
                if(tokens.size() == maxParts - 1)
                {
                    tokens.push_back(src.substr(lastPos));    
                    break;
                }
                else
                {
                    tokens.push_back(src.substr(lastPos, pos - lastPos));    
                }
            }

            lastPos = pos + 1;
        }

        return tokens;
    }
   
    uint32_t StringUtil::hash(const string& str)
    {
        //use elf hash
        uint32_t h = 0; 
        uint32_t x = 0;
        uint32_t i = 0;
        uint32_t len = (uint32_t)str.size();
        for(i = 0; i < len; ++i)
        {
            h = (h << 4) + str[i];
            if((x = h & 0xF0000000L) != 0)
            {
                h ^= (x >> 24);    
                h &= ~x;
            }    
        } 

        return (h & 0x7FFFFFFF);
    }
   
    string StringUtil::lower(const string& src)
    {
        string dest(src);
        
        transform(dest.begin(), dest.end(), dest.begin(), ::tolower);
        return dest;    
    }

    string StringUtil::upper(const string& src)
    {
        string dest(src);
        
        transform(dest.begin(), dest.end(), dest.begin(), ::toupper);
        return dest;    
    }

    string StringUtil::lstrip(const string& src)
    {
        string s(src);
        s.erase(s.begin(), find_if(s.begin(), s.end(), not1(ptr_fun<int, int>(isspace))));
        return s;
    } 

    string StringUtil::rstrip(const string& src)
    {
        string s(src);
        s.erase(find_if(s.rbegin(), s.rend(), not1(ptr_fun<int, int>(isspace))).base(), s.end());
        return s;    
    }
    
    string StringUtil::strip(const string& src)
    {
        return lstrip(rstrip(src));    
    }
    string StringUtil::hex(const uint8_t* src, size_t srcLen)
    {
        size_t destLen = 2 * srcLen;    
        string dest(destLen, '\0');

        static uint8_t h[] = "0123456789abcdef";

        for(size_t i = 0; i < srcLen; i++)
        {
            dest[i] = h[src[i] >> 4];
            dest[i + 1] = h[src[i + 1] & 0xf];                
        }

        return dest;
    }
   
    string StringUtil::hex(const string& src)
    {
        return hex((const uint8_t*)src.data(), src.size());     
    }
    
    string StringUtil::base64Encode(const string& src)
    {
        size_t destLen = src.size() * 4 / 3 + 4;
        string str(destLen, '\0');

        uint8_t* dest = (uint8_t*)&str[0];
        
        if(base64_encode(dest, &destLen, (const uint8_t*)src.data(), src.size()) != 0)
        {
            return "";    
        }

        str.resize(destLen);

        return str;
    }
    
    string StringUtil::base64Decode(const string& src)
    {
        size_t destLen = src.size() * 3 / 4 + 4;
        string str(destLen, '\0');

        uint8_t* dest = (uint8_t*)&str[0];
        
        if(base64_decode(dest, &destLen, (const uint8_t*)src.data(), src.size()) != 0)
        {
            return "";    
        }

        str.resize(destLen);

        return str;
    }
   
    string StringUtil::md5Bin(const string& src)
    {
        uint8_t output[16];
        md5((const uint8_t*)src.data(), src.size(), output);
        
        return string((char*)output, 16);
    }
    
    string StringUtil::md5Hex(const string& src)
    {
        uint8_t output[16];
        md5((const uint8_t*)src.data(), src.size(), output);
        
        return hex(output, 16);         
    } 

    string StringUtil::sha1Bin(const string& src)
    {
        uint8_t output[20];
        sha1((const uint8_t*)src.data(), src.size(), output);    
        return string((char*)output, 20);
    }

    string StringUtil::sha1Hex(const string& src)
    {
        uint8_t output[20];
        sha1((const uint8_t*)src.data(), src.size(), output);    
        return hex(output, 20);
    }
}
