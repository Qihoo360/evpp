#include "wsutil.h"

#include <string.h>
#include <stdio.h>

#include "httprequest.h"
#include "httpresponse.h"
#include "../stringutil.h"
#include "../logging.h"

using namespace std;

namespace evpp {

namespace xhttp { 

static const string upgradeKey = "Upgrade";
static const string upgradeValue = "websocket";
static const string connectionKey = "Connection";
static const string connectionValue = "Upgrade";
static const string wsVersionKey = "Sec-Websocket-Version";
static const string wsVersionValue = "13";
static const string wsProtocolKey = "Sec-Websocket-Protocol";
static const string wsProtocolValue = "chat";
static const string wsAcceptKey = "Sec-Websocket-Accept";
static const string wsKey = "Sec-Websocket-Key";
static const string originKey = "Origin";
static const string wsMagicKey = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";

string calcKey()
{
    int key[4];
    for(int i = 0; i < 4; ++i)
    {
        key[i] = (int)random();    
    }    

    return StringUtil::base64Encode(string((char*)key, 16)); 
}

string getOrigin(HttpRequest& request)
{
    string str;
    char buffer[1024];
    int n = 0;
    request.parseUrl();
    if(strcasecmp(request.schema.c_str(), "ws") == 0)
    {
        n = snprintf(buffer, sizeof(buffer), "http://%s", request.host.c_str());
    } 
    else if(strcasecmp(request.schema.c_str(), "wss") == 0)
    { 
        n = snprintf(buffer, sizeof(buffer), "https://%s", request.host.c_str());
    }
    else
    {
        n = snprintf(buffer, sizeof(buffer), "%s://%s", request.schema.c_str(), request.host.c_str());
    }

    str.append(buffer, n);

    if(request.port != 80 && request.port != 443)
    {
        n = snprintf(buffer, sizeof(buffer), ":%d", request.port);
        str.append(buffer, n);    
    }

    return str;
}

int WsUtil::buildRequest(HttpRequest& req)
{
    req.headers.insert(make_pair(upgradeKey, upgradeValue));
    req.headers.insert(make_pair(connectionKey, connectionValue));

    req.headers.insert(make_pair(wsProtocolKey, wsProtocolValue));
    
    req.headers.insert(make_pair(wsVersionKey, wsVersionValue));
    req.headers.insert(make_pair(wsKey, calcKey())); 
    req.headers.insert(make_pair(originKey, getOrigin(req)));
    return 0;
}

HttpError checkHeader(const HttpRequest& request)
{
    if(request.method != HTTP_GET)
    {
        return HttpError(405);    
    }
    
    auto iter = request.headers.find(upgradeKey);
    if(iter == request.headers.end() || strcasestr(iter->second.c_str(), upgradeValue.c_str()) == NULL)
    {
        return HttpError(400, "Can \"Upgrade\" only to \"websocket\"");
    }

    iter = request.headers.find(connectionKey);
    if(iter == request.headers.end() || strcasestr(iter->second.c_str(), connectionValue.c_str()) == NULL)
    {
        return HttpError(400, "\"Connection\" must be \"upgrade\"");
    }

    iter = request.headers.find(wsVersionKey);
    bool validVersion = false;
    if(iter != request.headers.end())
    {
        int version = atoi(iter->second.c_str());
        if(version == 13 || version == 7 || version == 8)
        {
            validVersion = true;    
        }    
    }

    if(!validVersion)
    {
        return HttpError(426, "Sec-WebSocket-Version: 13");
    }

    return HttpError(200);
}

static string calcAcceptKey(const HttpRequest& request)
{
    auto iter = request.headers.find(wsKey);
    if(iter == request.headers.end())
    {
        return "";
    }  

    string key = iter->second + wsMagicKey;

    return StringUtil::base64Encode(StringUtil::sha1Bin(key));
}

HttpError WsUtil::handshake(const HttpRequest& request, HttpResponse& resp)
{
    HttpError error = checkHeader(request);
    if(error.statusCode != 200)
    {
        return error;    
    }

    resp.statusCode = 101;
    resp.headers.insert(make_pair(upgradeKey, upgradeValue));
    resp.headers.insert(make_pair(connectionKey, upgradeKey));
    resp.headers.insert(make_pair(wsAcceptKey, calcAcceptKey(request)));

    auto iter = request.headers.find(wsProtocolKey);
    
    if(iter != request.headers.end())
    {
        static string sep = ",";
        vector<string> subs = StringUtil::split(iter->second, sep);

        //now we only choose first subprotocol
        if(!subs.empty())
        {
            resp.headers.insert(make_pair(wsProtocolKey, subs[0]));
        }
    }

    return error;
}

}
}