#pragma once

#include "evpp/httpc/request.h"

namespace evmc {
enum {
    READ_VBUCKET_CONF_FAILED = -1,
};
struct HttpReqDoneArg {
    event_base* event;
    std::string* retstr;
    int retcode;
};
class GetVbucketConf {
public:
    static int GetVbucketConfContext(const std::string& conf_addr, std::string& context);
private:
    static void OnHttpReqDone(struct evhttp_request* req, void* arg);
    static int GetRemoteVbucketConf(const std::string& conf_addr, std::string& context);
};
}