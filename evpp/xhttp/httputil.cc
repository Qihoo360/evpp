#include "httputil.h"
#include <vector>
#include <string>
#include "../stringutil.h"

extern "C"
{
#include "http_parser.h"    
}

using namespace std;

namespace evpp
{
    namespace xhttp { 
    static string unknown = "Unknown Error";
    
    class Reasons
    {
    public:
        Reasons()
        {
            m_reasons.resize(600);  
            
            m_reasons[100] = "Continue";
            m_reasons[101] = "Switching Protocols";
            m_reasons[200] = "OK";
            m_reasons[201] = "Created";
            m_reasons[202] = "Accepted";
            m_reasons[203] = "Non-Authoritative Information";
            m_reasons[204] = "No Content";
            m_reasons[205] = "Reset Content";
            m_reasons[206] = "Partial Content";
            m_reasons[300] = "Multiple Choices";
            m_reasons[301] = "Moved Permanently";
            m_reasons[302] = "Found";
            m_reasons[303] = "See Other";
            m_reasons[304] = "Not Modified";
            m_reasons[305] = "Use Proxy";
            m_reasons[307] = "Temporary Redirect";
            m_reasons[400] = "Bad Request";
            m_reasons[401] = "Unauthorized";
            m_reasons[402] = "Payment Required";
            m_reasons[403] = "Forbidden";
            m_reasons[404] = "Not Found";
            m_reasons[405] = "Method Not Allowed";
            m_reasons[406] = "Not Acceptable";
            m_reasons[407] = "Proxy Authentication Required";
            m_reasons[408] = "Request Time-out";
            m_reasons[409] = "Conflict";
            m_reasons[410] = "Gone";
            m_reasons[411] = "Length Required";
            m_reasons[412] = "Precondition Failed";
            m_reasons[413] = "Request Entity Too Large";
            m_reasons[414] = "Request-URI Too Large";
            m_reasons[415] = "Unsupported Media Type";
            m_reasons[416] = "Requested range not satisfiable";
            m_reasons[417] = "Expectation Failed";
            m_reasons[500] = "Internal Server Error";
            m_reasons[501] = "Not Implemented";
            m_reasons[502] = "Bad Gateway";
            m_reasons[503] = "Service Unavailable";
            m_reasons[504] = "Gateway Time-out";
            m_reasons[505] = "HTTP Version not supported";            
        }

        const string& getReason(int code)
        {
            if((size_t)code >= m_reasons.size())
            {
                return unknown;
            }     

            return m_reasons[code].empty() ? unknown : m_reasons[code];
        }

    private:
        vector<string> m_reasons;
    };

    static Reasons reason;

    const string& HttpUtil::codeReason(int code)
    {
        return reason.getReason(code);    
    }

    const char* HttpUtil::methodStr(unsigned char method)
    {
        return http_method_str((http_method)method);    
    }
   
    //refer to http://www.codeguru.com/cpp/cpp/algorithms/strings/article.php/c12759/URI-Encoding-and-Decoding.htm
    
    const char HEX2DEC[256] = 
    {
        /*       0  1  2  3   4  5  6  7   8  9  A  B   C  D  E  F */
        /* 0 */ -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
        /* 1 */ -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
        /* 2 */ -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
        /* 3 */  0, 1, 2, 3,  4, 5, 6, 7,  8, 9,-1,-1, -1,-1,-1,-1,
        
        /* 4 */ -1,10,11,12, 13,14,15,-1, -1,-1,-1,-1, -1,-1,-1,-1,
        /* 5 */ -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
        /* 6 */ -1,10,11,12, 13,14,15,-1, -1,-1,-1,-1, -1,-1,-1,-1,
        /* 7 */ -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
        
        /* 8 */ -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
        /* 9 */ -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
        /* A */ -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
        /* B */ -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
        
        /* C */ -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
        /* D */ -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
        /* E */ -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
        /* F */ -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1
    };
        
    std::string HttpUtil::unescape(const std::string& src)
    {
        //for a esacep character, is %xx, min size is 3
        if(src.size() <= 2)
        {
            return src;    
        }

        // Note from RFC1630:  "Sequences which start with a percent sign
        // but are not followed by two hexadecimal characters (0-9, A-F) are reserved
        // for future extension"
        
        string dest(src.size(), '\0');

        size_t i = 0;
        size_t j = 0;
        while(i < src.size() - 2)
        {
            if(src[i] == '%')
            {
                char dec1 = HEX2DEC[(int)src[i + 1]];
                char dec2 = HEX2DEC[(int)src[i + 2]];
                if(dec1 != -1 && dec2 != -1)
                {
                    dest[j++] = (dec1 << 4) + dec2;
                    i += 3;
                    continue;    
                }           
            }    

            dest[j++] = src[i++];
        }

        while(i < src.size())
        {
            dest[j++] = src[i++];    
        }

        dest.resize(j);

        return dest;
    }

    // Only alphanum is safe.
    const char SAFE[256] =
    {
        /*      0 1 2 3  4 5 6 7  8 9 A B  C D E F */
        /* 0 */ 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
        /* 1 */ 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
        /* 2 */ 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
        /* 3 */ 1,1,1,1, 1,1,1,1, 1,1,0,0, 0,0,0,0,
        
        /* 4 */ 0,1,1,1, 1,1,1,1, 1,1,1,1, 1,1,1,1,
        /* 5 */ 1,1,1,1, 1,1,1,1, 1,1,1,0, 0,0,0,0,
        /* 6 */ 0,1,1,1, 1,1,1,1, 1,1,1,1, 1,1,1,1,
        /* 7 */ 1,1,1,1, 1,1,1,1, 1,1,1,0, 0,0,0,0,
        
        /* 8 */ 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
        /* 9 */ 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
        /* A */ 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
        /* B */ 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
        
        /* C */ 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
        /* D */ 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
        /* E */ 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
        /* F */ 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0
    };

    std::string HttpUtil::escape(const std::string & src)
    {
        if(src.empty())
        {
            return ""; 
        }

        const char DEC2HEX[16 + 1] = "0123456789ABCDEF";

        string dest(src.size() * 3, '\0');

        size_t i = 0;
        size_t j = 0;
        for(;i < src.size(); ++i)
        {
            if(SAFE[(int)src[i]])
            {
                dest[j++] = src[i];    
            }    
            else
            {
                dest[j++] = '%';
                dest[j++] = DEC2HEX[src[i] >> 4];
                dest[j++] = DEC2HEX[src[i] & 0x0F];    
            }
        }

        dest.resize(j);
        return dest;
    }

    string HttpUtil::normalizeHeader(const string& src)
    {
        if(src.empty())
        {
            return src; 
        }
        
        string dest = StringUtil::lower(src);
        dest[0] = toupper(dest[0]);
        
        for(size_t i = 1; i < dest.size(); ++i)
        {
            if(dest[i - 1] == '-')
            {
                dest[i] = toupper(dest[i]);    
            }    
        }

        return dest;
    }
}

}