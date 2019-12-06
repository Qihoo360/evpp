#include  "evpp/evpphttp/http_request.h"
namespace evpp {
namespace evpphttp {
HttpRequest::HttpRequest() {
    settings.on_message_begin = &HttpRequest::OnMessageBegin;
    settings.on_message_complete = &HttpRequest::OnMessageEnd;
    settings.on_header_field = &HttpRequest::OnField;
    settings.on_header_value = &HttpRequest::OnValue;
    settings.on_body = &HttpRequest::OnBody;
    settings.on_url = &HttpRequest::OnUrl;
    settings.on_headers_complete = &HttpRequest::OnHeaderComplete;
    settings.on_reason = &HttpRequest::EmptyDataCB;
    settings.on_chunk_header = &HttpRequest::EmptyCB;
    settings.on_chunk_complete = &HttpRequest::EmptyCB;
    http_parser_init(&parser, HTTP_REQUEST);
}

int HttpRequest::Parse(evpp::Buffer * buf) {
    parser.data = const_cast<HttpRequest *>(this);
    size_t parsed = http_parser_execute(&parser, &settings, buf->data(), buf->size());
    auto err = HTTP_PARSER_ERRNO(&parser);
    if (err != HPE_OK && err != HPE_PAUSED) {
        LOG_WARN << "http request header parsed failed, err=" << http_errno_name(err) << "," << http_errno_description(err);
        return err;
    }
    buf->Retrieve(parsed);
    return 0;
}
}
}

