#pragma once

#include <string>
#include <map>

#include "xhttp.h"

namespace evpp {

class Buffer;

namespace xhttp { 
    class HttpResponse
    {
    public:
        HttpResponse();

        HttpResponse(int code, const Headers_t& headers = Headers_t(), const std::string& body = "");
        ~HttpResponse();    

        void clear();

        void setContentType(const std::string& contentType);
        void setKeepAlive(bool on);

        void enableDate();

        //generate http response text
        std::string dump();

        std::string dumpHeaders();
   
        int statusCode;

        
        Headers_t headers;

        //std::string body;
        void appendBody(const void* p, size_t len);
        void appendBody(const std::string& s);

        const evpp::Buffer* getBody() const {
            return body_;
        }
    private:
        evpp::Buffer* body_;
    };
    
}

}