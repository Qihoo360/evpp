#include "httpparser.h"

#include "httputil.h"

#include "../logging.h"
#include "xhttp.h"

using namespace std;

namespace evpp
{

namespace xhttp {
    struct http_parser_settings ms_settings;

    class HttpParserSettings
    {
    public:
        HttpParserSettings();

        static int onMessageBegin(struct http_parser*);
        static int onUrl(struct http_parser*, const char*, size_t);
        static int onStatusComplete(struct http_parser*);
        static int onHeaderField(struct http_parser*, const char*, size_t);
        static int onHeaderValue(struct http_parser*, const char*, size_t);
        static int onHeadersComplete(struct http_parser*);
        static int onBody(struct http_parser*, const char*, size_t);
        static int onMessageComplete(struct http_parser*); 
    };

    HttpParserSettings::HttpParserSettings()
    {
        ms_settings.on_message_begin = &HttpParserSettings::onMessageBegin;
        ms_settings.on_url = &HttpParserSettings::onUrl;
        ms_settings.on_status_complete = &HttpParserSettings::onStatusComplete;
        ms_settings.on_header_field = &HttpParserSettings::onHeaderField;
        ms_settings.on_header_value = &HttpParserSettings::onHeaderValue;
        ms_settings.on_headers_complete = &HttpParserSettings::onHeadersComplete;
        ms_settings.on_body = &HttpParserSettings::onBody;
        ms_settings.on_message_complete = &HttpParserSettings::onMessageComplete;    
    }    

    static HttpParserSettings initObj;

    int HttpParserSettings::onMessageBegin(struct http_parser* parser)
    {
        HttpParser* p = (HttpParser*)parser->data;
        return p->onParser(HttpParser::Parser_MessageBegin, 0, 0);
    }

    int HttpParserSettings::onUrl(struct http_parser* parser, const char* at, size_t length)
    {
        HttpParser* p = (HttpParser*)parser->data;
        return p->onParser(HttpParser::Parser_Url, at, length);
    }

    int HttpParserSettings::onStatusComplete(struct http_parser* parser)
    {
        HttpParser* p = (HttpParser*)parser->data;
        return p->onParser(HttpParser::Parser_StatusComplete, 0, 0);
    }

    int HttpParserSettings::onHeaderField(struct http_parser* parser, const char* at, size_t length)
    {
        HttpParser* p = (HttpParser*)parser->data;
        return p->onParser(HttpParser::Parser_HeaderField, at, length);
    }

    int HttpParserSettings::onHeaderValue(struct http_parser* parser, const char* at, size_t length)
    {
        HttpParser* p = (HttpParser*)parser->data;
        return p->onParser(HttpParser::Parser_HeaderValue, at, length);
    }

    int HttpParserSettings::onHeadersComplete(struct http_parser* parser)
    {
        HttpParser* p = (HttpParser*)parser->data;
        return p->onParser(HttpParser::Parser_HeadersComplete, 0, 0);
    }

    int HttpParserSettings::onBody(struct http_parser* parser, const char* at, size_t length)
    {
        HttpParser* p = (HttpParser*)parser->data;
        return p->onParser(HttpParser::Parser_Body, at, length);
    }

    int HttpParserSettings::onMessageComplete(struct http_parser* parser)
    {
        HttpParser* p = (HttpParser*)parser->data;
        return p->onParser(HttpParser::Parser_MessageComplete, 0, 0);
    }


    HttpParser::HttpParser(enum http_parser_type type)
    {
        http_parser_init(&m_parser, type);

        m_parser.data = this;
   
        m_lastWasValue = true;
    }
   
    HttpParser::~HttpParser()
    {
        
    }

    int HttpParser::onParser(Event event, const char* at, size_t length)
    {
        switch(event)
        {
            case Parser_MessageBegin:
                return handleMessageBegin();
            case Parser_Url:
                return onUrl(at, length);
            case Parser_StatusComplete:
                return 0;
            case Parser_HeaderField:
                return handleHeaderField(at, length);
            case Parser_HeaderValue:
                return handleHeaderValue(at, length);
            case Parser_HeadersComplete:
                return handleHeadersComplete();
            case Parser_Body:
                return onBody(at, length);
            case Parser_MessageComplete:
                return onMessageComplete();
            default:
                break;
        }

        return 0;
    }

    int HttpParser::handleMessageBegin()
    {
        m_curField.clear();
        m_curValue.clear();
        m_lastWasValue = true;
        
        m_errorCode = 0;

        return onMessageBegin();
    }        
        
    int HttpParser::handleHeaderField(const char* at, size_t length)
    {
        if(m_lastWasValue)
        {
            if(!m_curField.empty())
            {  
                onHeader(HttpUtil::normalizeHeader(m_curField), m_curValue);
            }
            
            m_curField.clear();    
            m_curValue.clear();
        }

        m_curField.append(at, length);

        m_lastWasValue = 0;

        return 0;
    }
        
    int HttpParser::handleHeaderValue(const char* at, size_t length)
    {
        m_curValue.append(at, length);
        m_lastWasValue = 1;

        return 0;
    }
        
    int HttpParser::handleHeadersComplete()
    {
        if(!m_curField.empty())
        {
            string field = HttpUtil::normalizeHeader(m_curField); 
            onHeader(field, m_curValue);    
        }    

        return onHeadersComplete();
    }

    int HttpParser::execute(const char* buffer, size_t count)
    {
        size_t n = http_parser_execute(&m_parser, &ms_settings, buffer, count);
        if(m_parser.upgrade)
        {
            onUpgrade(buffer + n, count - n); 
            return 0;
        }
        else if(n != count)
        {
            int code = (m_errorCode != 0 ? m_errorCode : 400);
            
            evpp::xhttp::HttpError error(code, http_errno_description((http_errno)m_parser.http_errno));

            LOG_ERROR << "parser error " << error.message;
            
            onError(error);

            return code;
        }     

        return 0;
    }
}

}