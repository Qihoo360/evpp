#include <iostream>

#include "test_common.h"

#include <stdio.h>
#include <stdlib.h>

#include <evpp/libevent.h>
#include <evpp/timestamp.h>
#include <evpp/event_loop_thread.h>

#include <evpp/httpc/request.h>
#include <evpp/httpc/conn.h>
#include <evpp/httpc/response.h>

#include "evpp/evpphttp/service.h"
#include "evpp/evpphttp/http_request.h"
#include "evpp/evpphttp/http_response.h"
using namespace evpp::evpphttp;

TEST_UNIT(testHttpRequest1) {
	HttpRequest hr;
	evpp::Buffer buf;
	buf.Append(
			"GET /forums/1/topics/2375?page=1#posts-17408 HTTP/1.1\r\n"
			"Host: 0.0.0.0=5000\r\n"
			"User-Agent: Mozilla/5.0 (X11; U; Linux i686; en-US; rv:1.9) Gecko/2008061015 Firefox/3.0\r\n"
			"Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8\r\n"
			"Accept-Language: en-us,en;q=0.5\r\n"
			"Accept-Encoding: gzip,deflate\r\n"
			"Accept-Charset: ISO-8859-1,utf-8;q=0.7,*;q=0.7\r\n"
			"Keep-Alive: 300\r\n"
			"Connection: keep-alive\r\n"
			"\r\n"
			);
    hr.Parse(&buf);
    H_TEST_ASSERT(hr.completed());
    EXPECT_EQ(hr.field_value.size(), 8);
    H_TEST_ASSERT(hr.field_value["Connection"].compare("keep-alive") == 0);
    H_TEST_ASSERT(hr.field_value["Accept-Language"].compare("en-us,en;q=0.5") == 0);
    H_TEST_ASSERT(hr.url_query() == "page=1");
    H_TEST_ASSERT(hr.url_fragment() == "posts-17408");
	ASSERT_STREQ(hr.url_path().c_str(), "/forums/1/topics/2375");
}

TEST_UNIT(testHttpRequest2) {
	HttpRequest hr;
	evpp::Buffer buf;
    //just for test
	const char * raw = "POST /post_identity_body_world?q=search#hey HTTP/1.1\r\n"
		"Host: www.example.com\r\n"
		"Content-Type: application/example\r\n"                                                                                                                                                                                                                                            "If-Match: \"e0023aa4e\"\r\n"
		   "Content-";
	buf.Append(raw);
    hr.Parse(&buf);
    H_TEST_ASSERT(!hr.completed());
	evpp::Buffer buf1;
    H_TEST_ASSERT(!hr.completed());
	buf1.Append(
		   "Length: 10\r\n"
		    "field:t"
    );
    hr.Parse(&buf1);
    H_TEST_ASSERT(!hr.completed());
	evpp::Buffer buf2;
	buf2.Append("est\r\n\r\ntestttestt");
    hr.Parse(&buf2);
    H_TEST_ASSERT(hr.completed());
	ASSERT_STREQ(hr.field_value["Content-Length"].c_str(), "10");
	ASSERT_STREQ(hr.field_value["field"].c_str(), "test");
	ASSERT_STREQ(hr.url_path().c_str(), "/post_identity_body_world");
	ASSERT_STREQ(hr.body.ToString().c_str(), "testttestt");
}


