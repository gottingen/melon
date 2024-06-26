//
// Copyright (C) 2024 EA group inc.
// Author: Jeff.li lijippy@163.com
// All rights reserved.
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as published
// by the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.
//
//


#include <gtest/gtest.h>
#include <iostream>

#include <melon/utility/time.h>
#include <turbo/log/logging.h>
#include <melon/rpc/http/http_parser.h>
#include <melon/builtin/common.h>  // AppendFileName

using melon::http_parser;
using melon::http_parser_init;
using melon::http_parser_settings;

class HttpParserTest : public testing::Test {
protected:
    void SetUp() {}
    void TearDown() {}
};

TEST_F(HttpParserTest, init_perf) {
    const size_t loops = 10000000;
    mutil::Timer timer;
    timer.start();
    for (size_t i = 0; i < loops; ++i) {
        http_parser parser;
        http_parser_init(&parser, melon::HTTP_REQUEST);
    }
    timer.stop();
    std::cout << "It takes " << timer.n_elapsed() / loops
              << "ns to init a http_parser"
              << std::endl;
}

int on_message_begin(http_parser *) {
    LOG(INFO) << "Start parsing message";
    return 0;
}

int on_url(http_parser *, const char *at, const size_t length) {
    LOG(INFO) << "Get url " << std::string(at, length);
    return 0;
}

int on_headers_complete(http_parser *) {
    LOG(INFO) << "Header complete";
    return 0;
}

int on_message_complete(http_parser *) {
    LOG(INFO) << "Message complete";
    return 0;
}

int on_header_field(http_parser *, const char *at, const size_t length) {
    LOG(INFO) << "Get header field " << std::string(at, length);
    return 0;
}

int on_header_value(http_parser *, const char *at, const size_t length) {
    LOG(INFO) << "Get header value " << std::string(at, length);
    return 0;
}

int on_body(http_parser *, const char *at, const size_t length) {
    LOG(INFO) << "Get body " << std::string(at, length);
    return 0;
}

TEST_F(HttpParserTest, http_example) {
    const char *http_request = 
        "GET /path/file.html?sdfsdf=sdfs HTTP/1.0\r\n"
        "From: someuser@jmarshall.com\r\n"
        "User-Agent: HTTPTool/1.0\r\n"
        "Content-Type: json\r\n"
        "Content-Length: 19\r\n"
        "Host: sdlfjslfd\r\n"
        "Accept: */*\r\n"
        "\r\n"
        "Message Body sdfsdf\r\n"
    ;
    std::cout << http_request << std::endl;

    http_parser parser;
    http_parser_init(&parser, melon::HTTP_REQUEST);
    http_parser_settings settings;
    memset(&settings, 0, sizeof(settings));
    settings.on_message_begin = on_message_begin;
    settings.on_url = on_url;
    settings.on_headers_complete = on_headers_complete;
    settings.on_message_complete = on_message_complete;
    settings.on_header_field = on_header_field;
    settings.on_header_value = on_header_value;
    settings.on_body = on_body;
    LOG(INFO) << http_parser_execute(&parser, &settings, http_request, strlen(http_request));
}

TEST_F(HttpParserTest, append_filename) {
    std::string dir;

    dir = "/home/someone/.bsvn/..";
    melon::AppendFileName(&dir, "..");
    ASSERT_EQ("/home", dir);

    dir = "/home/someone/.bsvn/../";
    melon::AppendFileName(&dir, "..");
    ASSERT_EQ("/home", dir);

    dir = "/home/someone/./..";
    melon::AppendFileName(&dir, "..");
    ASSERT_EQ("/", dir);
    
    dir = "/home/someone/./../";
    melon::AppendFileName(&dir, "..");
    ASSERT_EQ("/", dir);

    dir = "/foo/bar";
    melon::AppendFileName(&dir, "..");
    ASSERT_EQ("/foo", dir);

    dir = "/foo/bar/";
    melon::AppendFileName(&dir, "..");
    ASSERT_EQ("/foo", dir);

    dir = "/foo";
    melon::AppendFileName(&dir, ".");
    ASSERT_EQ("/foo", dir);

    dir = "/foo/";
    melon::AppendFileName(&dir, ".");
    ASSERT_EQ("/foo/", dir);

    dir = "foo";
    melon::AppendFileName(&dir, "..");
    ASSERT_EQ("", dir);

    dir = "foo/";
    melon::AppendFileName(&dir, "..");
    ASSERT_EQ("", dir);

    dir = "foo/..";
    melon::AppendFileName(&dir, "..");
    ASSERT_EQ("..", dir);

    dir = "foo/../";
    melon::AppendFileName(&dir, "..");
    ASSERT_EQ("..", dir);
    
    dir = "/foo";
    melon::AppendFileName(&dir, "..");
    ASSERT_EQ("/", dir);

    dir = "/foo/";
    melon::AppendFileName(&dir, "..");
    ASSERT_EQ("/", dir);
}
