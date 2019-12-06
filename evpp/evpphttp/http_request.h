#pragma once
#include "evpp/buffer.h"
#include "evpp/evpphttp/http_parser.h"
#include <map>
namespace evpp {
namespace evpphttp {
class HttpRequest {
public:
    inline bool completed() const {
        return is_completed;
    }
    HttpRequest();
    HttpRequest(HttpRequest & hr) {
        swap(hr);
    }
    HttpRequest(HttpRequest&& hr) {
        swap(hr);
    }
    HttpRequest(const HttpRequest & hr) = delete;
    int Parse(evpp::Buffer * buf);
    std::string url_path() {
        if ((u.field_set & (1 << UF_PATH)) != 0) {
            return std::string(url, u.field_data[3].off,  u.field_data[3].len);
        }
        return "";
    }
    std::string url_query() {
        if ((u.field_set & (1 << UF_QUERY)) != 0) {
            return std::string(url, u.field_data[4].off,  u.field_data[4].len);
        }
        return "";
    }
    std::string url_fragment() {
        if ((u.field_set & (1 << UF_FRAGMENT)) != 0) {
            return std::string(url, u.field_data[5].off,  u.field_data[5].len);
        }
        return "";
    }
    std::string url_userinfo() {
        if ((u.field_set & (1 << UF_USERINFO)) != 0) {
            return std::string(url, u.field_data[6].off,  u.field_data[6].len);
        }
        return "";
    }
    void set_remote_ip(const std::string& ip) {
        remote_ip.assign(ip);
    }
    inline bool is_send_continue() const {
        return send_continue_;
    }
    inline void set_continue() {
        send_continue_ = true;
    }
public:
    evpp::Buffer body;
    std::map<std::string, std::string> field_value;
    evpp::http_parser parser;
    std::string remote_ip;

private:
    void swap(HttpRequest & hr) {
        body.Swap(hr.body);
        field_value.swap(hr.field_value);
        parser = hr.parser;
        remote_ip.swap(hr.remote_ip);
        field.swap(hr.field);
        value.swap(hr.value);
        url.swap(hr.url);
        pre_state = hr.pre_state;
        is_completed = hr.is_completed;
        settings = hr.settings;
        send_continue_ = hr.send_continue_;
        u = hr.u;
    }
    static int OnMessageBegin(http_parser *p) {
        return 0;
    }

    static int OnMessageEnd(http_parser *p) {
        auto req = static_cast<HttpRequest *>(p->data);
        req->is_completed = true;
        evpp::http_parser_pause(p, 1);
        return 0;
    }

    static int OnUrl(http_parser *p, const char *buf, size_t len) {
        auto req = static_cast<HttpRequest *>(p->data);
        req->url.append(buf, len);
        return 0;
    }

    static int OnField(http_parser *p, const char *buf, size_t len) {
        auto req = static_cast<HttpRequest *>(p->data);
        if (req->pre_state == 51) {
            req->field.append(buf, len);
        } else {
            req->field.assign(buf, len);
        }
        req->pre_state = p->state;
        return 0;
    }

    static int OnValue(http_parser *p, const char *buf, size_t len) {
        auto req = static_cast<HttpRequest *>(p->data);
        if (req->pre_state == 53/**/) {
            req->value.append(buf, len);
        } else {
            req->value.assign(buf, len);
        }

        unsigned char state = p->state;
        if (state == 55/*head value*/) {
            req->field_value[req->field] = std::move(req->value);
            req->field.clear();
        }
        req->pre_state = state;
        return 0;
    }

    static int OnBody(http_parser *p, const char *buf, size_t len) {
        auto req = static_cast<HttpRequest *>(p->data);
        req->body.Append(buf, len);
        return 0;
    }

    static int OnHeaderComplete(http_parser *p, const char *buf, size_t len) {
        auto req = static_cast<HttpRequest *>(p->data);
        evpp::http_parser_parse_url(req->url.data(), req->url.size(), 1, &req->u);
        return 0;
    }

    static int EmptyCB(http_parser *p) {
        return 0;
    }

    static int EmptyDataCB(http_parser *p, const char *buf, size_t len) {
        return 0;
    }
private:
    std::string field;
    std::string value;
    std::string url;
    unsigned char pre_state{0};
    bool is_completed{false};
    bool send_continue_{false};
    http_parser_settings settings;
    http_parser_url  u;
};
}
}

