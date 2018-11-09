#pragma once

#include <string>
#include <map>
#include <stdint.h>

extern "C"
{
#include "http_parser.h"
}

#include "xhttp.h"

namespace evpp
{
class Buffer;

namespace xhttp {
class HttpRequest
{
public:
    HttpRequest();
    ~HttpRequest();

    void clear();
    void parseUrl();



    std::string url;    

    std::string schema;
    
    std::string host;
    std::string path;
    std::string query;

    Headers_t headers;

    Params_t params;
    
    unsigned short majorVersion;
    unsigned short minorVersion;

    http_method method;

    uint16_t port;

    void parseQuery();

    std::string dump();
    std::string dumpHeaders();

    int bodyLen() const;

    void appendBody(const void* data, const size_t len);
    void appendBody(const std::string& str);
private:
    evpp::Buffer* body_;
};
        
}

}