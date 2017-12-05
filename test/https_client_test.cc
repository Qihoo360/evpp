#include <iostream>
#include <chrono>
#include <thread>

#include "test_common.h"

#include <evpp/event_loop_thread.h>
#include <evpp/httpc/request.h>
#include <evpp/httpc/conn.h>
#include <evpp/httpc/response.h>
#include <evpp/httpc/ssl.h>

namespace {
void InitSSLOnce() {
  static std::once_flag flag;
  std::call_once(flag, [](){ evpp::httpc::InitSSL(); });
}

std::string HttpFetch(const std::string& url) {
  InitSSLOnce();
  evpp::EventLoopThread t;
  t.Start(true);
  evpp::httpc::GetRequest* req =
      new evpp::httpc::GetRequest(t.loop(), url, evpp::Duration(1.0));
  volatile bool responsed = false;
  std::string ret;
  req->Execute([req, &ret, &responsed](
                const std::shared_ptr<evpp::httpc::Response>& response) mutable {
    std::stringstream oss;
    oss << "http_code="
        << response->http_code()
        << std::endl
        << "body ["
        << std::endl
        << response->body().ToString()
        << "]"
        << std::endl;
    responsed = true;
    delete req;
    ret = oss.str();
  });
  while (!responsed) {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }
  t.Stop(true);
  return ret;
}
}

TEST_UNIT(testHTTPResponse) {
  std::string response = HttpFetch("http://httpbin.org/headers?show_env=1");
  H_TEST_ASSERT(!response.empty());
  H_TEST_ASSERT(response.find("http_code=200") != std::string::npos);
  H_TEST_ASSERT(response.find("\"Host\": \"httpbin.org\",") != std::string::npos);
  H_TEST_ASSERT(response.find("\"X-Forwarded-Port\": \"80\",") != std::string::npos);
  H_TEST_ASSERT(response.find("\"X-Forwarded-Proto\": \"http\",") != std::string::npos);
  H_TEST_ASSERT(response.find("\"Connection\": \"close\",") != std::string::npos);
}

TEST_UNIT(testHTTPSResponse) {
  std::string response = HttpFetch("https://httpbin.org/headers?show_env=1");
  H_TEST_ASSERT(!response.empty());
  H_TEST_ASSERT(response.find("http_code=200") != std::string::npos);
  H_TEST_ASSERT(response.find("\"Host\": \"httpbin.org\",") != std::string::npos);
  H_TEST_ASSERT(response.find("\"X-Forwarded-Port\": \"443\",") != std::string::npos);
  H_TEST_ASSERT(response.find("\"X-Forwarded-Proto\": \"https\",") != std::string::npos);
  H_TEST_ASSERT(response.find("\"Connection\": \"close\",") != std::string::npos);
}
