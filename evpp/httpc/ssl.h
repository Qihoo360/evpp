#pragma once

#if defined(EVPP_HTTP_CLIENT_SUPPORTS_SSL)

#include <openssl/ssl.h>

namespace evpp {
namespace httpc {
bool InitSSL();
void CleanSSL();
SSL_CTX* GetSSLCtx();
} // httpc
} // evpp

#endif
