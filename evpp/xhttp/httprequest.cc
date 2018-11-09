#include "httprequest.h"

#include <string.h>
#include <stdlib.h>
#include <vector>

extern "C"
{
#include "http_parser.h"    
}

#include "httputil.h"
#include <evpp/stringutil.h>
#include <evpp/logging.h>
#include <evpp/buffer.h>

using namespace std;

namespace evpp
{
namespace xhttp { 

    HttpRequest::HttpRequest()
    : body_(nullptr)   {
        majorVersion = 1;
        minorVersion = 1;
        method = HTTP_GET;
        body_ = new evpp::Buffer();
    }
   
    HttpRequest::~HttpRequest() {
        if (body_) {
            delete body_;
            body_ = nullptr;
        }
    } 

    void HttpRequest::clear() {
        url.clear();
        schema.clear();
        host.clear();
        path.clear();
        query.clear();
        body_->Reset();
        
        headers.clear();
        params.clear();
        
        majorVersion = 1;
        minorVersion = 1;
        method = HTTP_GET;        
        port = 80;
    }

    void HttpRequest::parseUrl()
    {
        if(!schema.empty())
        {
            return;    
        }

        struct http_parser_url u;
        if(http_parser_parse_url(url.c_str(), url.size(), 0, &u) != 0)
        {
            LOG_ERROR << "parseurl error " << url;
            return;    
        }

        if(u.field_set & (1 << UF_SCHEMA))
        {
            schema = url.substr(u.field_data[UF_SCHEMA].off, u.field_data[UF_SCHEMA].len);    
        }
        
        if(u.field_set & (1 << UF_HOST))
        {
            host = url.substr(u.field_data[UF_HOST].off, u.field_data[UF_HOST].len);    
        }

        if(u.field_set & (1 << UF_PORT))
        {
            port = u.port;    
        }
        else
        {
            if(strcasecmp(schema.c_str(), "https") == 0 || strcasecmp(schema.c_str(), "wss") == 0)
            {
                port = 443;    
            }
            else
            {
                port = 80; 
            }   
        }

        if(u.field_set & (1 << UF_PATH))
        {
            path = url.substr(u.field_data[UF_PATH].off, u.field_data[UF_PATH].len);    
        }

        if(u.field_set & (1 << UF_QUERY))
        {
            query = url.substr(u.field_data[UF_QUERY].off, u.field_data[UF_QUERY].len);    
            parseQuery();
        } 
        
    }

    void HttpRequest::parseQuery()
    {
        if(query.empty() || !params.empty())
        {
            return;    
        }

        static string sep1 = "&";
        static string sep2 = "=";

        vector<string> args = StringUtil::split(query, sep1);
        string key;
        string value;
        for(size_t i = 0; i < args.size(); ++i)
        {
            vector<string> p = StringUtil::split(args[i], sep2);
            if(p.size() == 2)
            {
                key = p[0];
                value = p[1]; 
            }    
            else if(p.size() == 1)
            {
                key = p[0];
                value = "";    
            }
            else
            {
                //invalid, ignore
                continue;    
            }

            params.insert(make_pair(HttpUtil::unescape(key), HttpUtil::unescape(value)));
        }
           
    }

    static const string HostKey = "Host";
    static const string ContentLengthKey = "Content-Length";
    
    string HttpRequest::dump()
    {
        string str;
        
        parseUrl();
        
        char buf[1024];
        
        int n = 0;
        if(path.empty())
        {
            path = "/";
        }

        if(query.empty())
        {
            n = snprintf(buf, sizeof(buf), "%s %s HTTP/%d.%d\r\n", 
                http_method_str(method), path.c_str(), majorVersion, minorVersion);    
        }
        else
        {
            n = snprintf(buf, sizeof(buf), "%s %s?%s HTTP/%d.%d\r\n", 
                http_method_str(method), path.c_str(), query.c_str(), majorVersion, minorVersion);    
        }

        str.append(buf, n);

        headers.erase(HostKey);

        if(port == 80 || port == 443)
        {
            headers.insert(make_pair(HostKey, host));    
        }
        else
        {
            n = snprintf(buf, sizeof(buf), "%s:%d", host.c_str(), port);
            headers.insert(make_pair(HostKey, string(buf, n)));   
        }

        if(method == HTTP_POST || method == HTTP_PUT)
        {
            headers.erase(ContentLengthKey);

            n = snprintf(buf, sizeof(buf), "%d", int(body_->length()));
            headers.insert(make_pair(ContentLengthKey, string(buf, n)));
        }

        auto iter = headers.cbegin();
        while(iter != headers.cend())
        {
            int mm = snprintf(buf, sizeof(buf), "%s: %s\r\n", iter->first.c_str(), iter->second.c_str());
            str.append(buf, mm);
            ++iter;    
        }

        str.append("\r\n");

        str.append(body_->ToString());

        return str;
    } 

    int HttpRequest::bodyLen() const {
        return body_->length();
    }


    void HttpRequest::appendBody(const void* data, const size_t len) {
        if (body_) {
            body_->Write(data, len);
        }
    }


    void HttpRequest::appendBody(const std::string& str) {
        if (body_)
            body_->Write(str.data(), str.size());
    }
}
}