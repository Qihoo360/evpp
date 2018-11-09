#pragma once

#include "xhttp.h"

namespace evpp {

namespace xhttp { 

class WsUtil
{
public:
    static int buildRequest(HttpRequest& request);

    static HttpError handshake(const HttpRequest& request, HttpResponse& resp);
};

}
    
}
