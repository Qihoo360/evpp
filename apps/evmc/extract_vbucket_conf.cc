#include <fstream>
#include "evpp/httpc/url_parser.h"
#include "evpp/libevent.h"
#include "evpp/httpc/request.h"
#include "evpp/httpc/conn.h"
#include "evpp/httpc/response.h"
#include "extract_vbucket_conf.h"
namespace evmc {
int GetVbucketConf::GetVbucketConfContext(const std::string& conf_addr, std::string& context) {
    if (conf_addr.substr(0, 4) == "http") {
        return GetRemoteVbucketConf(conf_addr, context);
    }
    std::ifstream ifs;
    ifs.open(conf_addr.c_str(), std::ios::in);
    if (ifs.good()) {
        std::string line;
        while (getline(ifs, line)) {
            context.append(line);
            line.clear();
        }
        ifs.close();
    } else {
        LOG_WARN << "read local vbucket conf:" << conf_addr << " failed";
        return READ_VBUCKET_CONF_FAILED;
    }
    return 0;
}

void GetVbucketConf::OnHttpReqDone(struct evhttp_request* req, void* arg) {
    char buf[1024 * 1024] = {};
    HttpReqDoneArg* argument = (HttpReqDoneArg*)arg;
    int res_code = req->response_code;
    if (res_code == 200) {
        argument->retcode = 0;
    } else {
        argument->retcode = res_code;
        LOG_WARN << "http request to get remote vbucket conf ret=" << res_code;
        return;
    }
    evbuffer_remove(req->input_buffer, &buf, sizeof(buf) - 1);
    argument->retstr->assign(buf);
    event_base_loopbreak(argument->event);
}

int GetVbucketConf::GetRemoteVbucketConf(const std::string& conf_addr, std::string& context) {
    evpp::httpc::URLParser url(conf_addr);
    struct event_base* base = event_base_new();
    if (base == NULL) {
        return -1;
    }
    struct evhttp_connection* conn = evhttp_connection_base_new(base, nullptr, url.host.c_str(), url.port);
    if (conn == NULL) {
        return -1;
    }
    HttpReqDoneArg arg;
    arg.event = base;
    arg.retstr = &context;
    arg.retcode = -1;
    struct evhttp_request* req = evhttp_request_new(GetVbucketConf::OnHttpReqDone, (void*)&arg);
    evhttp_add_header(req->output_headers, "Host", url.host.c_str());
    evhttp_make_request(conn, req, EVHTTP_REQ_GET, url.path.c_str());
    evhttp_connection_set_timeout(req->evcon, 500);
    event_base_dispatch(base);
    evhttp_connection_free(conn);
    event_base_free(base);
    return arg.retcode;
}
}
