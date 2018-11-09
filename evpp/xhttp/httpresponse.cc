#include "httpresponse.h"

#include <string.h>
#include <stdio.h>
#include <time.h>
#include <evpp/buffer.h>

#include "httputil.h"

using namespace std;

namespace evpp {

namespace xhttp { 
    HttpResponse::HttpResponse()
        : statusCode(200)
    {
    }

    HttpResponse::HttpResponse(int code, const Headers_t& resp_headers, const string& resp_body)
        : statusCode(code)
        , headers(resp_headers)        
        , body_(new evpp::Buffer())
    {
        body_->Write(resp_body.data(), resp_body.size());
    }

    HttpResponse::~HttpResponse() {
        if (body_) {
            delete body_;
            body_ = nullptr;
        }
    }

    void HttpResponse::clear() {
        statusCode = 200;
        body_->Reset();
        headers.clear();    
    }

    std::string HttpResponse::dumpHeaders() {
        std::string str;
        
        char buf[1024];
        int n = snprintf(buf, sizeof(buf), "HTTP/1.1 %d %s\r\n", statusCode, HttpUtil::codeReason(statusCode).c_str());    

        str.append(buf, n);
    
        n = snprintf(buf, sizeof(buf), "%d", int(body_->length()));
        static const string ContentLength = "Content-Length";
        headers.insert(make_pair(ContentLength, string(buf, n)));

        auto it = headers.cbegin();
        while(it != headers.cend()) {
            n = snprintf(buf, sizeof(buf), "%s: %s\r\n", it->first.c_str(), it->second.c_str());
            str.append(buf, n);
            ++it;    
        }

        str.append("\r\n");
        return std::move(str);
    }

    string HttpResponse::dump() {
        std::string str = dumpHeaders();
        str.append(body_->ToString());

        return str;
    }     

    void HttpResponse::setContentType(const std::string& contentType)
    {
        static const string ContentTypeKey = "Content-Type";
        headers.insert(make_pair(ContentTypeKey, contentType));    
    }

    void HttpResponse::setKeepAlive(bool on)
    {
        static const string ConnectionKey = "Connection";
        if(on)
        {
            static const string KeepAliveValue = "Keep-Alive";
            headers.insert(make_pair(ConnectionKey, KeepAliveValue));    
        }    
        else
        {
            static const string CloseValue = "close";
            headers.insert(make_pair(ConnectionKey, CloseValue));    
        }
    }
    
    void HttpResponse::enableDate()
    {
        time_t now = time(NULL);
        struct tm t; 
        gmtime_r(&now, &t);
        char buf[128];
        int n = strftime(buf, sizeof buf, "%a, %d %b %Y %H:%M:%S %Z", &t);
        static const string DateKey = "Date";
        headers.insert(make_pair(DateKey, string(buf, n)));
    }

    void HttpResponse::appendBody(const void* p, size_t len) {
        if (body_) 
            body_->Write(p, len);
    }

    void HttpResponse::appendBody(const std::string& s) {
        if (body_)
            body_->Write(s.data(), s.size());
    }
}
}