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



#include <iostream>
#include <unordered_map>
#include <melon/utility/time.h>
#include <turbo/log/logging.h>
#include <melon/rpc/redis/redis.h>
#include <melon/rpc/channel.h>
#include <melon/rpc/policy/redis_authenticator.h>
#include <melon/rpc/server.h>
#include <melon/rpc/redis/redis_command.h>
#include <gtest/gtest.h>

namespace melon {
DECLARE_int32(idle_timeout_second);
}

int main(int argc, char* argv[]) {
    melon::FLAGS_idle_timeout_second = 0;
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

namespace {
static pthread_once_t download_redis_server_once = PTHREAD_ONCE_INIT;

static pid_t g_redis_pid = -1; 

static void RemoveRedisServer() {
    if (g_redis_pid > 0) {
        puts("[Stopping redis-server]");
        char cmd[256];
        snprintf(cmd, sizeof(cmd), "kill %d", g_redis_pid);
        CHECK(0 == system(cmd));
        // Wait for redis to stop
        usleep(50000);
    }
}

#define REDIS_SERVER_BIN "redis-server"
#define REDIS_SERVER_PORT "6479"

static void RunRedisServer() {
    if (system("which " REDIS_SERVER_BIN) != 0) {
        puts("Fail to find " REDIS_SERVER_BIN ", following tests will be skipped");
        return;
    }
    atexit(RemoveRedisServer);

    g_redis_pid = fork();
    if (g_redis_pid < 0) {
        puts("Fail to fork");
        exit(1);
    } else if (g_redis_pid == 0) {
        puts("[Starting redis-server]");
        char* const argv[] = { (char*)REDIS_SERVER_BIN,
                               (char*)"--port", (char*)REDIS_SERVER_PORT,
                               NULL };
        unlink("dump.rdb");
        if (execvp(REDIS_SERVER_BIN, argv) < 0) {
            puts("Fail to run " REDIS_SERVER_BIN);
            exit(1);
        }
    }
    // Wait for redis to start.
    usleep(50000);
}

class RedisTest : public testing::Test {
protected:
    RedisTest() {}
    void SetUp() {
        pthread_once(&download_redis_server_once, RunRedisServer);
    }
    void TearDown() {}
};

void AssertReplyEqual(const melon::RedisReply& reply1,
                      const melon::RedisReply& reply2) {
    if (&reply1 == &reply2) {
        return;
    }
    CHECK_EQ(reply1.type(), reply2.type());
    switch (reply1.type()) {
    case melon::REDIS_REPLY_ARRAY:
        ASSERT_EQ(reply1.size(), reply2.size());
        for (size_t j = 0; j < reply1.size(); ++j) {
            ASSERT_NE(&reply1[j], &reply2[j]); // from different arena
            AssertReplyEqual(reply1[j], reply2[j]);
        }
        break;
    case melon::REDIS_REPLY_INTEGER:
        ASSERT_EQ(reply1.integer(), reply2.integer());
        break;
    case melon::REDIS_REPLY_NIL:
        break;
    case melon::REDIS_REPLY_STRING:
        // fall through
    case melon::REDIS_REPLY_STATUS:
        ASSERT_NE(reply1.c_str(), reply2.c_str()); // from different arena
        ASSERT_EQ(reply1.data(), reply2.data());
        break;
    case melon::REDIS_REPLY_ERROR:
        ASSERT_NE(reply1.error_message(), reply2.error_message()); // from different arena
        ASSERT_STREQ(reply1.error_message(), reply2.error_message());
        break;
    }
}

void AssertResponseEqual(const melon::RedisResponse& r1,
                         const melon::RedisResponse& r2,
                         int repeated_times = 1) {
    if (&r1 == &r2) {
        ASSERT_EQ(repeated_times, 1);
        return;
    }
    ASSERT_EQ(r2.reply_size()* repeated_times, r1.reply_size());
    for (int j = 0; j < repeated_times; ++j) {
        for (int i = 0; i < r2.reply_size(); ++i) {
            ASSERT_NE(&r2.reply(i), &r1.reply(j * r2.reply_size() + i));
            AssertReplyEqual(r2.reply(i), r1.reply(j * r2.reply_size() + i));
        }
    }
}

TEST_F(RedisTest, sanity) {
    if (g_redis_pid < 0) {
        puts("Skipped due to absence of redis-server");
        return;
    }
    melon::ChannelOptions options;
    options.protocol = melon::PROTOCOL_REDIS;
    melon::Channel channel;
    ASSERT_EQ(0, channel.Init("0.0.0.0:" REDIS_SERVER_PORT, &options));
    melon::RedisRequest request;
    melon::RedisResponse response;
    melon::Controller cntl;

    ASSERT_TRUE(request.AddCommand("get hello"));
    channel.CallMethod(NULL, &cntl, &request, &response, NULL);
    ASSERT_FALSE(cntl.Failed()) << cntl.ErrorText();
    ASSERT_EQ(1, response.reply_size());
    ASSERT_EQ(melon::REDIS_REPLY_NIL, response.reply(0).type())
        << response;

    cntl.Reset();
    request.Clear();
    response.Clear();
    request.AddCommand("set hello world");
    channel.CallMethod(NULL, &cntl, &request, &response, NULL);
    ASSERT_FALSE(cntl.Failed()) << cntl.ErrorText();
    ASSERT_EQ(1, response.reply_size());
    ASSERT_EQ(melon::REDIS_REPLY_STATUS, response.reply(0).type());
    ASSERT_EQ("OK", response.reply(0).data());

    cntl.Reset();
    request.Clear();
    response.Clear();
    ASSERT_TRUE(request.AddCommand("get hello"));
    channel.CallMethod(NULL, &cntl, &request, &response, NULL);
    ASSERT_FALSE(cntl.Failed());
    ASSERT_EQ(1, response.reply_size());
    ASSERT_EQ(melon::REDIS_REPLY_STRING, response.reply(0).type());
    ASSERT_EQ("world", response.reply(0).data());

    cntl.Reset();
    request.Clear();
    response.Clear();
    request.AddCommand("set hello world2");
    channel.CallMethod(NULL, &cntl, &request, &response, NULL);
    ASSERT_FALSE(cntl.Failed()) << cntl.ErrorText();
    ASSERT_EQ(1, response.reply_size());
    ASSERT_EQ(melon::REDIS_REPLY_STATUS, response.reply(0).type());
    ASSERT_EQ("OK", response.reply(0).data());
    
    cntl.Reset();
    request.Clear();
    response.Clear();
    ASSERT_TRUE(request.AddCommand("get hello"));
    channel.CallMethod(NULL, &cntl, &request, &response, NULL);
    ASSERT_FALSE(cntl.Failed());
    ASSERT_EQ(1, response.reply_size());
    ASSERT_EQ(melon::REDIS_REPLY_STRING, response.reply(0).type());
    ASSERT_EQ("world2", response.reply(0).data());

    cntl.Reset();
    request.Clear();
    response.Clear();
    ASSERT_TRUE(request.AddCommand("del hello"));
    channel.CallMethod(NULL, &cntl, &request, &response, NULL);
    ASSERT_FALSE(cntl.Failed());
    ASSERT_EQ(melon::REDIS_REPLY_INTEGER, response.reply(0).type());
    ASSERT_EQ(1, response.reply(0).integer());

    cntl.Reset();
    request.Clear();
    response.Clear();
    ASSERT_TRUE(request.AddCommand("get %s", "hello"));
    channel.CallMethod(NULL, &cntl, &request, &response, NULL);
    ASSERT_FALSE(cntl.Failed()) << cntl.ErrorText();
    ASSERT_EQ(1, response.reply_size());
    ASSERT_EQ(melon::REDIS_REPLY_NIL, response.reply(0).type());
}

TEST_F(RedisTest, keys_with_spaces) {
    if (g_redis_pid < 0) {
        puts("Skipped due to absence of redis-server");
        return;
    }
    melon::ChannelOptions options;
    options.protocol = melon::PROTOCOL_REDIS;
    melon::Channel channel;
    ASSERT_EQ(0, channel.Init("0.0.0.0:" REDIS_SERVER_PORT, &options));
    melon::RedisRequest request;
    melon::RedisResponse response;
    melon::Controller cntl;
    
    cntl.Reset();
    request.Clear();
    response.Clear();
    ASSERT_TRUE(request.AddCommand("set %s 'he1 he1 da1'", "hello world"));
    ASSERT_TRUE(request.AddCommand("set 'hello2 world2' 'he2 he2 da2'"));
    ASSERT_TRUE(request.AddCommand("set \"hello3 world3\" \"he3 he3 da3\""));
    ASSERT_TRUE(request.AddCommand("get \"hello world\""));
    ASSERT_TRUE(request.AddCommand("get 'hello world'"));
    ASSERT_TRUE(request.AddCommand("get 'hello2 world2'"));
    ASSERT_TRUE(request.AddCommand("get 'hello3 world3'"));

    channel.CallMethod(NULL, &cntl, &request, &response, NULL);
    ASSERT_FALSE(cntl.Failed()) << cntl.ErrorText();
    ASSERT_EQ(7, response.reply_size());
    ASSERT_EQ(melon::REDIS_REPLY_STATUS, response.reply(0).type());
    ASSERT_EQ("OK", response.reply(0).data());
    ASSERT_EQ(melon::REDIS_REPLY_STATUS, response.reply(1).type());
    ASSERT_EQ("OK", response.reply(1).data());
    ASSERT_EQ(melon::REDIS_REPLY_STATUS, response.reply(2).type());
    ASSERT_EQ("OK", response.reply(2).data());
    ASSERT_EQ(melon::REDIS_REPLY_STRING, response.reply(3).type());
    ASSERT_EQ("he1 he1 da1", response.reply(3).data());
    ASSERT_EQ(melon::REDIS_REPLY_STRING, response.reply(4).type());
    ASSERT_EQ("he1 he1 da1", response.reply(4).data());
    ASSERT_EQ(melon::REDIS_REPLY_STRING, response.reply(5).type());
    ASSERT_EQ("he2 he2 da2", response.reply(5).data());
    ASSERT_EQ(melon::REDIS_REPLY_STRING, response.reply(6).type());
    ASSERT_EQ("he3 he3 da3", response.reply(6).data());

    melon::RedisResponse response2 = response;
    AssertResponseEqual(response2, response);
    response2.MergeFrom(response);
    AssertResponseEqual(response2, response, 2);
}

TEST_F(RedisTest, incr_and_decr) {
    if (g_redis_pid < 0) {
        puts("Skipped due to absence of redis-server");
        return;
    }
    melon::ChannelOptions options;
    options.protocol = melon::PROTOCOL_REDIS;
    melon::Channel channel;
    ASSERT_EQ(0, channel.Init("0.0.0.0:" REDIS_SERVER_PORT, &options));
    melon::RedisRequest request;
    melon::RedisResponse response;
    melon::Controller cntl;

    request.AddCommand("incr counter1");
    request.AddCommand("decr counter1");
    request.AddCommand("incrby counter1 %d", 10);
    request.AddCommand("decrby counter1 %d", 20);
    channel.CallMethod(NULL, &cntl, &request, &response, NULL);
    ASSERT_FALSE(cntl.Failed()) << cntl.ErrorText();
    ASSERT_EQ(4, response.reply_size());
    ASSERT_EQ(melon::REDIS_REPLY_INTEGER, response.reply(0).type());
    ASSERT_EQ(1, response.reply(0).integer());
    ASSERT_EQ(melon::REDIS_REPLY_INTEGER, response.reply(1).type());
    ASSERT_EQ(0, response.reply(1).integer());
    ASSERT_EQ(melon::REDIS_REPLY_INTEGER, response.reply(2).type());
    ASSERT_EQ(10, response.reply(2).integer());
    ASSERT_EQ(melon::REDIS_REPLY_INTEGER, response.reply(3).type());
    ASSERT_EQ(-10, response.reply(3).integer());

    melon::RedisResponse response2 = response;
    AssertResponseEqual(response2, response);
    response2.MergeFrom(response);
    AssertResponseEqual(response2, response, 2);
}

TEST_F(RedisTest, by_components) {
    if (g_redis_pid < 0) {
        puts("Skipped due to absence of redis-server");
        return;
    }
    melon::ChannelOptions options;
    options.protocol = melon::PROTOCOL_REDIS;
    melon::Channel channel;
    ASSERT_EQ(0, channel.Init("0.0.0.0:" REDIS_SERVER_PORT, &options));
    melon::RedisRequest request;
    melon::RedisResponse response;
    melon::Controller cntl;

    mutil::StringPiece comp1[] = { "incr", "counter2" };
    mutil::StringPiece comp2[] = { "decr", "counter2" };
    mutil::StringPiece comp3[] = { "incrby", "counter2", "10" };
    mutil::StringPiece comp4[] = { "decrby", "counter2", "20" };

    request.AddCommandByComponents(comp1, arraysize(comp1));
    request.AddCommandByComponents(comp2, arraysize(comp2));
    request.AddCommandByComponents(comp3, arraysize(comp3));
    request.AddCommandByComponents(comp4, arraysize(comp4));

    channel.CallMethod(NULL, &cntl, &request, &response, NULL);
    ASSERT_FALSE(cntl.Failed()) << cntl.ErrorText();
    ASSERT_EQ(4, response.reply_size());
    ASSERT_EQ(melon::REDIS_REPLY_INTEGER, response.reply(0).type());
    ASSERT_EQ(1, response.reply(0).integer());
    ASSERT_EQ(melon::REDIS_REPLY_INTEGER, response.reply(1).type());
    ASSERT_EQ(0, response.reply(1).integer());
    ASSERT_EQ(melon::REDIS_REPLY_INTEGER, response.reply(2).type());
    ASSERT_EQ(10, response.reply(2).integer());
    ASSERT_EQ(melon::REDIS_REPLY_INTEGER, response.reply(3).type());
    ASSERT_EQ(-10, response.reply(3).integer());

    melon::RedisResponse response2 = response;
    AssertResponseEqual(response2, response);
    response2.MergeFrom(response);
    AssertResponseEqual(response2, response, 2);
}

static std::string GeneratePassword() {
    std::string result;
    result.reserve(12);
    for (size_t i = 0; i < result.capacity(); ++i) {
        result.push_back(mutil::fast_rand_in('a', 'z'));
    }
    return result;
}

TEST_F(RedisTest, auth) {
    if (g_redis_pid < 0) {
        puts("Skipped due to absence of redis-server");
        return;
    }
    // generate a random password
    const std::string passwd1 = GeneratePassword();
    const std::string passwd2 = GeneratePassword();
    LOG(INFO) << "Generated passwd1=" << passwd1 << " passwd2=" << passwd2;

    // config auth
    {
        melon::ChannelOptions options;
        options.protocol = melon::PROTOCOL_REDIS;
        melon::Channel channel;
        ASSERT_EQ(0, channel.Init("0.0.0.0:" REDIS_SERVER_PORT, &options));
        melon::RedisRequest request;
        melon::RedisResponse response;
        melon::Controller cntl;

        request.AddCommand("set mykey %s", passwd1.c_str());
        request.AddCommand("config set requirepass %s", passwd1.c_str());
        request.AddCommand("auth %s", passwd1.c_str());
        request.AddCommand("get mykey");

        channel.CallMethod(NULL, &cntl, &request, &response, NULL);
        ASSERT_FALSE(cntl.Failed()) << cntl.ErrorText();
        ASSERT_EQ(4, response.reply_size());
        ASSERT_EQ(melon::REDIS_REPLY_STATUS, response.reply(0).type());
        ASSERT_STREQ("OK", response.reply(0).c_str());
        ASSERT_EQ(melon::REDIS_REPLY_STATUS, response.reply(1).type());
        ASSERT_STREQ("OK", response.reply(1).c_str());
        ASSERT_EQ(melon::REDIS_REPLY_STATUS, response.reply(2).type());
        ASSERT_STREQ("OK", response.reply(2).c_str());
        ASSERT_EQ(melon::REDIS_REPLY_STRING, response.reply(3).type());
        ASSERT_STREQ(passwd1.c_str(), response.reply(3).c_str());
    }

    // Auth failed
    {
        melon::ChannelOptions options;
        options.protocol = melon::PROTOCOL_REDIS;
        melon::Channel channel;
        ASSERT_EQ(0, channel.Init("0.0.0.0:" REDIS_SERVER_PORT, &options));
        melon::RedisRequest request;
        melon::RedisResponse response;
        melon::Controller cntl;

        request.AddCommand("get mykey");
        channel.CallMethod(NULL, &cntl, &request, &response, NULL);
        ASSERT_FALSE(cntl.Failed()) << cntl.ErrorText();
        ASSERT_EQ(1, response.reply_size());
        ASSERT_EQ(melon::REDIS_REPLY_ERROR, response.reply(0).type());
    }

    // Auth with RedisAuthenticator and change to passwd2 (setting to empty
    // pass does not work on redis 6.0.6)
    {
        melon::ChannelOptions options;
        options.protocol = melon::PROTOCOL_REDIS;
        melon::Channel channel;
        melon::policy::RedisAuthenticator* auth =
          new melon::policy::RedisAuthenticator(passwd1.c_str());
        options.auth = auth;
        ASSERT_EQ(0, channel.Init("0.0.0.0:" REDIS_SERVER_PORT, &options));
        melon::RedisRequest request;
        melon::RedisResponse response;
        melon::Controller cntl;

        request.AddCommand("get mykey");
        request.AddCommand("config set requirepass %s", passwd2.c_str());

        channel.CallMethod(NULL, &cntl, &request, &response, NULL);
        ASSERT_FALSE(cntl.Failed()) << cntl.ErrorText();
        ASSERT_EQ(2, response.reply_size());
        ASSERT_EQ(melon::REDIS_REPLY_STRING, response.reply(0).type());
        ASSERT_STREQ(passwd1.c_str(), response.reply(0).c_str());
        ASSERT_EQ(melon::REDIS_REPLY_STATUS, response.reply(1).type());
        ASSERT_STREQ("OK", response.reply(1).c_str());
    }

    // Auth with passwd2
    {
        melon::ChannelOptions options;
        options.protocol = melon::PROTOCOL_REDIS;
        melon::policy::RedisAuthenticator* auth =
          new melon::policy::RedisAuthenticator(passwd2.c_str());
        options.auth = auth;
        melon::Channel channel;
        ASSERT_EQ(0, channel.Init("0.0.0.0:" REDIS_SERVER_PORT, &options));
        melon::RedisRequest request;
        melon::RedisResponse response;
        melon::Controller cntl;

        request.AddCommand("get mykey");
        channel.CallMethod(NULL, &cntl, &request, &response, NULL);
        ASSERT_FALSE(cntl.Failed()) << cntl.ErrorText();
        ASSERT_EQ(1, response.reply_size());
        ASSERT_EQ(melon::REDIS_REPLY_STRING, response.reply(0).type()) << response.reply(0);
        ASSERT_STREQ(passwd1.c_str(), response.reply(0).c_str());
    }
}

TEST_F(RedisTest, cmd_format) {
    if (g_redis_pid < 0) {
        puts("Skipped due to absence of redis-server");
        return;
    }
    melon::RedisRequest request;
    // set empty string
    request.AddCommand("set a ''");
    ASSERT_STREQ("*3\r\n$3\r\nset\r\n$1\r\na\r\n$0\r\n\r\n", 
		request._buf.to_string().c_str());
    request.Clear();

    request.AddCommand("mset b '' c ''");
    ASSERT_STREQ("*5\r\n$4\r\nmset\r\n$1\r\nb\r\n$0\r\n\r\n$1\r\nc\r\n$0\r\n\r\n",
		request._buf.to_string().c_str());
    request.Clear();
    // set non-empty string
    request.AddCommand("set a 123");
    ASSERT_STREQ("*3\r\n$3\r\nset\r\n$1\r\na\r\n$3\r\n123\r\n", 
		request._buf.to_string().c_str());
    request.Clear();

    request.AddCommand("mset b '' c ccc");
    ASSERT_STREQ("*5\r\n$4\r\nmset\r\n$1\r\nb\r\n$0\r\n\r\n$1\r\nc\r\n$3\r\nccc\r\n",
		request._buf.to_string().c_str());
    request.Clear();

    request.AddCommand("get ''key value");  // == get <empty> key value
    ASSERT_STREQ("*4\r\n$3\r\nget\r\n$0\r\n\r\n$3\r\nkey\r\n$5\r\nvalue\r\n", request._buf.to_string().c_str());
    request.Clear();

    request.AddCommand("get key'' value");  // == get key <empty> value
    ASSERT_STREQ("*4\r\n$3\r\nget\r\n$3\r\nkey\r\n$0\r\n\r\n$5\r\nvalue\r\n", request._buf.to_string().c_str());
    request.Clear();

    request.AddCommand("get 'ext'key   value  ");  // == get ext key value
    ASSERT_STREQ("*4\r\n$3\r\nget\r\n$3\r\next\r\n$3\r\nkey\r\n$5\r\nvalue\r\n", request._buf.to_string().c_str());
    request.Clear();
    
    request.AddCommand("  get   key'ext'   value  ");  // == get key ext value
    ASSERT_STREQ("*4\r\n$3\r\nget\r\n$3\r\nkey\r\n$3\r\next\r\n$5\r\nvalue\r\n", request._buf.to_string().c_str());
    request.Clear();
}

TEST_F(RedisTest, quote_and_escape) {
    if (g_redis_pid < 0) {
        puts("Skipped due to absence of redis-server");
        return;
    }
    melon::RedisRequest request;
    request.AddCommand("set a 'foo bar'");
    ASSERT_STREQ("*3\r\n$3\r\nset\r\n$1\r\na\r\n$7\r\nfoo bar\r\n",
                 request._buf.to_string().c_str());
    request.Clear();

    request.AddCommand("set a 'foo \\'bar'");
    ASSERT_STREQ("*3\r\n$3\r\nset\r\n$1\r\na\r\n$8\r\nfoo 'bar\r\n",
                 request._buf.to_string().c_str());
    request.Clear();

    request.AddCommand("set a 'foo \"bar'");
    ASSERT_STREQ("*3\r\n$3\r\nset\r\n$1\r\na\r\n$8\r\nfoo \"bar\r\n",
                 request._buf.to_string().c_str());
    request.Clear();

    request.AddCommand("set a 'foo \\\"bar'");
    ASSERT_STREQ("*3\r\n$3\r\nset\r\n$1\r\na\r\n$9\r\nfoo \\\"bar\r\n",
                 request._buf.to_string().c_str());
    request.Clear();

    request.AddCommand("set a \"foo 'bar\"");
    ASSERT_STREQ("*3\r\n$3\r\nset\r\n$1\r\na\r\n$8\r\nfoo 'bar\r\n",
                 request._buf.to_string().c_str());
    request.Clear();

    request.AddCommand("set a \"foo \\'bar\"");
    ASSERT_STREQ("*3\r\n$3\r\nset\r\n$1\r\na\r\n$9\r\nfoo \\'bar\r\n",
                 request._buf.to_string().c_str());
    request.Clear();

    request.AddCommand("set a \"foo \\\"bar\"");
    ASSERT_STREQ("*3\r\n$3\r\nset\r\n$1\r\na\r\n$8\r\nfoo \"bar\r\n",
                 request._buf.to_string().c_str());
    request.Clear();
}

std::string GetCompleteCommand(const std::vector<mutil::StringPiece>& commands) {
	std::string res;
    for (int i = 0; i < (int)commands.size(); ++i) {
        if (i != 0) {
            res.push_back(' ');
        }
        res.append(commands[i].data(), commands[i].size());
    }
    return res;
}


TEST_F(RedisTest, command_parser) {
    melon::RedisCommandParser parser;
    mutil::IOBuf buf;
    std::vector<mutil::StringPiece> command_out;
    mutil::Arena arena;
    {
        // parse from whole command
        std::string command = "set abc edc";
        ASSERT_TRUE(melon::RedisCommandNoFormat(&buf, command.c_str()).ok());
        ASSERT_EQ(melon::PARSE_OK, parser.Consume(buf, &command_out, &arena));
        ASSERT_TRUE(buf.empty());
        ASSERT_EQ(command, GetCompleteCommand(command_out));
    }
    {
        // simulate parsing from network
        int t = 100;
        std::string raw_string("*3\r\n$3\r\nset\r\n$3\r\nabc\r\n$3\r\ndef\r\n");
        int size = raw_string.size();
        while (t--) {
            for (int i = 0; i < size; ++i) {
                buf.push_back(raw_string[i]);
                if (i == size - 1) {
                    ASSERT_EQ(melon::PARSE_OK, parser.Consume(buf, &command_out, &arena));
                } else {
                    if (mutil::fast_rand_less_than(2) == 0) {
                        ASSERT_EQ(melon::PARSE_ERROR_NOT_ENOUGH_DATA,
                                parser.Consume(buf, &command_out, &arena));
                    }
                }
            }
            ASSERT_TRUE(buf.empty());
            ASSERT_EQ(GetCompleteCommand(command_out), "set abc def");
        }
    }
    {
        // there is a non-string message in command and parse should fail
        buf.append("*3\r\n$3");
        ASSERT_EQ(melon::PARSE_ERROR_NOT_ENOUGH_DATA, parser.Consume(buf, &command_out, &arena));
        ASSERT_EQ((int)buf.size(), 2);    // left "$3"
        buf.append("\r\nset\r\n:123\r\n$3\r\ndef\r\n");
        ASSERT_EQ(melon::PARSE_ERROR_ABSOLUTELY_WRONG, parser.Consume(buf, &command_out, &arena));
        parser.Reset();
    }
    {
        // not array
        buf.append(":123456\r\n");
        ASSERT_EQ(melon::PARSE_ERROR_TRY_OTHERS, parser.Consume(buf, &command_out, &arena));
        parser.Reset();
    }
    {
        // not array
        buf.append("+Error\r\n");
        ASSERT_EQ(melon::PARSE_ERROR_TRY_OTHERS, parser.Consume(buf, &command_out, &arena));
        parser.Reset();
    }
    {
        // not array
        buf.append("+OK\r\n");
        ASSERT_EQ(melon::PARSE_ERROR_TRY_OTHERS, parser.Consume(buf, &command_out, &arena));
        parser.Reset();
    }
    {
        // not array
        buf.append("$5\r\nhello\r\n");
        ASSERT_EQ(melon::PARSE_ERROR_TRY_OTHERS, parser.Consume(buf, &command_out, &arena));
        parser.Reset();
    }
}

TEST_F(RedisTest, redis_reply_codec) {
    mutil::Arena arena;
    // status
    {
        melon::RedisReply r(&arena);
        mutil::IOBuf buf;
        mutil::IOBufAppender appender;
        r.SetStatus("OK");
        ASSERT_TRUE(r.SerializeTo(&appender));
        appender.move_to(buf);
        ASSERT_STREQ(buf.to_string().c_str(), "+OK\r\n");
        ASSERT_STREQ(r.c_str(), "OK");

        melon::RedisReply r2(&arena);
        melon::ParseError err = r2.ConsumePartialIOBuf(buf);
        ASSERT_EQ(err, melon::PARSE_OK);
        ASSERT_TRUE(r2.is_string());
        ASSERT_STREQ("OK", r2.c_str());
    }
    // error
    {
        melon::RedisReply r(&arena);
        mutil::IOBuf buf;
        mutil::IOBufAppender appender;
        r.SetError("not exist \'key\'");
        ASSERT_TRUE(r.SerializeTo(&appender));
        appender.move_to(buf);
        ASSERT_STREQ(buf.to_string().c_str(), "-not exist \'key\'\r\n");

        melon::RedisReply r2(&arena);
        melon::ParseError err = r2.ConsumePartialIOBuf(buf);
        ASSERT_EQ(err, melon::PARSE_OK);
        ASSERT_TRUE(r2.is_error());
        ASSERT_STREQ("not exist \'key\'", r2.error_message());
    }
    // string
    {
        melon::RedisReply r(&arena);
        mutil::IOBuf buf;
        mutil::IOBufAppender appender;
        r.SetNullString();
        ASSERT_TRUE(r.SerializeTo(&appender));
        appender.move_to(buf);
        ASSERT_STREQ(buf.to_string().c_str(), "$-1\r\n");
        
        melon::RedisReply r2(&arena);
        melon::ParseError err = r2.ConsumePartialIOBuf(buf);
        ASSERT_EQ(err, melon::PARSE_OK);
        ASSERT_TRUE(r2.is_nil());

        r.SetString("abcde'hello world");
        ASSERT_TRUE(r.SerializeTo(&appender));
        appender.move_to(buf);
        ASSERT_STREQ(buf.to_string().c_str(), "$17\r\nabcde'hello world\r\n");
        ASSERT_STREQ("abcde'hello world", r.c_str());

        r.FormatString("int:%d str:%s fp:%.2f", 123, "foobar", 3.21);
        ASSERT_TRUE(r.SerializeTo(&appender));
        appender.move_to(buf);
        ASSERT_STREQ(buf.to_string().c_str(), "$26\r\nint:123 str:foobar fp:3.21\r\n");
        ASSERT_STREQ("int:123 str:foobar fp:3.21", r.c_str());

        r.FormatString("verylongstring verylongstring verylongstring verylongstring int:%d str:%s fp:%.2f", 123, "foobar", 3.21);
        ASSERT_TRUE(r.SerializeTo(&appender));
        appender.move_to(buf);
        ASSERT_STREQ(buf.to_string().c_str(), "$86\r\nverylongstring verylongstring verylongstring verylongstring int:123 str:foobar fp:3.21\r\n");
        ASSERT_STREQ("verylongstring verylongstring verylongstring verylongstring int:123 str:foobar fp:3.21", r.c_str());
        
        melon::RedisReply r3(&arena);
        err = r3.ConsumePartialIOBuf(buf);
        ASSERT_EQ(err, melon::PARSE_OK);
        ASSERT_TRUE(r3.is_string());
        ASSERT_STREQ(r.c_str(), r3.c_str());
    }
    // integer
    {
        melon::RedisReply r(&arena);
        mutil::IOBuf buf;
        mutil::IOBufAppender appender;
        int t = 2;
        int input[] = { -1, 1234567 };
        const char* output[] = { ":-1\r\n", ":1234567\r\n" };
        for (int i = 0; i < t; ++i) {
            r.SetInteger(input[i]);
            ASSERT_TRUE(r.SerializeTo(&appender));
            appender.move_to(buf);
            ASSERT_STREQ(buf.to_string().c_str(), output[i]);

            melon::RedisReply r2(&arena);
            melon::ParseError err = r2.ConsumePartialIOBuf(buf);
            ASSERT_EQ(err, melon::PARSE_OK);
            ASSERT_TRUE(r2.is_integer());
            ASSERT_EQ(r2.integer(), input[i]);
        }
    }
    // array
    {
        melon::RedisReply r(&arena);
        mutil::IOBuf buf;
        mutil::IOBufAppender appender;
        r.SetArray(3);
        melon::RedisReply& sub_reply = r[0];
        sub_reply.SetArray(2);
        sub_reply[0].SetString("hello, it's me");
        sub_reply[1].SetInteger(422);
        r[1].SetString("To go over everything");
        r[2].SetInteger(1);
        ASSERT_TRUE(r[3].is_nil());
        ASSERT_TRUE(r.SerializeTo(&appender));
        appender.move_to(buf);
        ASSERT_STREQ(buf.to_string().c_str(),
                "*3\r\n*2\r\n$14\r\nhello, it's me\r\n:422\r\n$21\r\n"
                "To go over everything\r\n:1\r\n");

        melon::RedisReply r2(&arena);
        ASSERT_EQ(r2.ConsumePartialIOBuf(buf), melon::PARSE_OK);
        ASSERT_TRUE(r2.is_array());
        ASSERT_EQ(3ul, r2.size());
        ASSERT_TRUE(r2[0].is_array());
        ASSERT_EQ(2ul, r2[0].size());
        ASSERT_TRUE(r2[0][0].is_string());
        ASSERT_STREQ(r2[0][0].c_str(), "hello, it's me");
        ASSERT_TRUE(r2[0][1].is_integer());
        ASSERT_EQ(r2[0][1].integer(), 422);
        ASSERT_TRUE(r2[1].is_string());
        ASSERT_STREQ(r2[1].c_str(), "To go over everything");
        ASSERT_TRUE(r2[2].is_integer());
        ASSERT_EQ(1, r2[2].integer());

        // null array
        r.SetNullArray();
        ASSERT_TRUE(r.SerializeTo(&appender));
        appender.move_to(buf);
        ASSERT_STREQ(buf.to_string().c_str(), "*-1\r\n");
        ASSERT_EQ(r.ConsumePartialIOBuf(buf), melon::PARSE_OK);
        ASSERT_TRUE(r.is_nil());
    }

    // CopyFromDifferentArena
    {
        melon::RedisReply r(&arena);
        r.SetArray(1);
        melon::RedisReply& sub_reply = r[0];
        sub_reply.SetArray(2);
        sub_reply[0].SetString("hello, it's me");
        sub_reply[1].SetInteger(422);

        melon::RedisReply r2(&arena);
        r2.CopyFromDifferentArena(r);
        ASSERT_TRUE(r2.is_array());
        ASSERT_EQ((int)r2[0].size(), 2);
        ASSERT_STREQ(r2[0][0].c_str(), sub_reply[0].c_str());
        ASSERT_EQ(r2[0][1].integer(), sub_reply[1].integer());
    }
    // SetXXX can be called multiple times.
    {
        melon::RedisReply r(&arena);
        r.SetStatus("OK");
        ASSERT_TRUE(r.is_string());
        r.SetNullString();
        ASSERT_TRUE(r.is_nil());
        r.SetArray(2);
        ASSERT_TRUE(r.is_array());
        r.SetString("OK");
        ASSERT_TRUE(r.is_string());
        r.SetError("OK");
        ASSERT_TRUE(r.is_error());
        r.SetInteger(42);
        ASSERT_TRUE(r.is_integer());
    }
}

mutil::Mutex s_mutex;
std::unordered_map<std::string, std::string> m;
std::unordered_map<std::string, int64_t> int_map;

class RedisServiceImpl : public melon::RedisService {
public:
    RedisServiceImpl()
        : _batch_count(0) {}

    melon::RedisCommandHandlerResult OnBatched(const std::vector<mutil::StringPiece>& args,
                   melon::RedisReply* output, bool flush_batched) {
        if (_batched_command.empty() && flush_batched) {
            if (args[0] == "set") {
                DoSet(args[1].as_string(), args[2].as_string(), output);
            } else if (args[0] == "get") {
                DoGet(args[1].as_string(), output);
            }
            return melon::REDIS_CMD_HANDLED;
        }
        std::vector<std::string> comm;
        for (int i = 0; i < (int)args.size(); ++i) {
            comm.push_back(args[i].as_string());
        }
        _batched_command.push_back(comm);
        if (flush_batched) {
            output->SetArray(_batched_command.size());
            for (int i = 0; i < (int)_batched_command.size(); ++i) {
                if (_batched_command[i][0] == "set") {
                    DoSet(_batched_command[i][1], _batched_command[i][2], &(*output)[i]);
                } else if (_batched_command[i][0] == "get") {
                    DoGet(_batched_command[i][1], &(*output)[i]);
                }
            }
            _batch_count++;
            _batched_command.clear();
            return melon::REDIS_CMD_HANDLED;
        } else {
            return melon::REDIS_CMD_BATCHED;
        }
    }

    void DoSet(const std::string& key, const std::string& value, melon::RedisReply* output) {
        m[key] = value;
        output->SetStatus("OK");
    }

    void DoGet(const std::string& key, melon::RedisReply* output) {
        auto it = m.find(key);
        if (it != m.end()) {
            output->SetString(it->second);
        } else {
            output->SetNullString();
        }
    }

    std::vector<std::vector<std::string> > _batched_command;
    int _batch_count;
};


class SetCommandHandler : public melon::RedisCommandHandler {
public:
    SetCommandHandler(RedisServiceImpl* rs, bool batch_process = false)
        : _rs(rs)
        , _batch_process(batch_process) {}

    melon::RedisCommandHandlerResult Run(const std::vector<mutil::StringPiece>& args,
                                        melon::RedisReply* output,
                                        bool flush_batched) {
        if (args.size() < 3) {
            output->SetError("ERR wrong number of arguments for 'set' command");
            return melon::REDIS_CMD_HANDLED;
        }
        if (_batch_process) {
            return _rs->OnBatched(args, output, flush_batched);
        } else {
            DoSet(args[1].as_string(), args[2].as_string(), output);
            return melon::REDIS_CMD_HANDLED;
        }
    }

    void DoSet(const std::string& key, const std::string& value, melon::RedisReply* output) {
        m[key] = value;
        output->SetStatus("OK");
    }

private:
    RedisServiceImpl* _rs;
    bool _batch_process;
};

class GetCommandHandler : public melon::RedisCommandHandler {
public:
    GetCommandHandler(RedisServiceImpl* rs, bool batch_process = false)
        : _rs(rs)
        , _batch_process(batch_process) {}

    melon::RedisCommandHandlerResult Run(const std::vector<mutil::StringPiece>& args,
                                        melon::RedisReply* output,
                                        bool flush_batched) {
        if (args.size() < 2) {
            output->SetError("ERR wrong number of arguments for 'get' command");
            return melon::REDIS_CMD_HANDLED;
        }
        if (_batch_process) {
            return _rs->OnBatched(args, output, flush_batched);
        } else {
            DoGet(args[1].as_string(), output);
            return melon::REDIS_CMD_HANDLED;
        }
    }

    void DoGet(const std::string& key, melon::RedisReply* output) {
        auto it = m.find(key);
        if (it != m.end()) {
            output->SetString(it->second);
        } else {
            output->SetNullString();
        }
    }

private:
    RedisServiceImpl* _rs;
    bool _batch_process;
};

class IncrCommandHandler : public melon::RedisCommandHandler {
public:
    IncrCommandHandler() {}

    melon::RedisCommandHandlerResult Run(const std::vector<mutil::StringPiece>& args,
                                        melon::RedisReply* output,
                                        bool flush_batched) {
        if (args.size() < 2) {
            output->SetError("ERR wrong number of arguments for 'incr' command");
            return melon::REDIS_CMD_HANDLED;
        }
        int64_t value;
        s_mutex.lock();
        value = ++int_map[args[1].as_string()];
        s_mutex.unlock();
        output->SetInteger(value);
        return melon::REDIS_CMD_HANDLED;
    }
};

TEST_F(RedisTest, server_sanity) {
    melon::Server server;
    melon::ServerOptions server_options;
    RedisServiceImpl* rsimpl = new RedisServiceImpl;
    GetCommandHandler *gh = new GetCommandHandler(rsimpl);
    SetCommandHandler *sh = new SetCommandHandler(rsimpl);
    IncrCommandHandler *ih = new IncrCommandHandler;
    rsimpl->AddCommandHandler("get", gh);
    rsimpl->AddCommandHandler("set", sh);
    rsimpl->AddCommandHandler("incr", ih);
    server_options.redis_service = rsimpl;
    melon::PortRange pr(8081, 8900);
    ASSERT_EQ(0, server.Start("127.0.0.1", pr, &server_options));

    melon::ChannelOptions options;
    options.protocol = melon::PROTOCOL_REDIS;
    melon::Channel channel;
    ASSERT_EQ(0, channel.Init("127.0.0.1", server.listen_address().port, &options));

    melon::RedisRequest request;
    melon::RedisResponse response;
    melon::Controller cntl;
    ASSERT_TRUE(request.AddCommand("get hello"));
    ASSERT_TRUE(request.AddCommand("get hello2"));
    ASSERT_TRUE(request.AddCommand("set key1 value1"));
    ASSERT_TRUE(request.AddCommand("get key1"));
    ASSERT_TRUE(request.AddCommand("set key2 value2"));
    ASSERT_TRUE(request.AddCommand("get key2"));
    ASSERT_TRUE(request.AddCommand("xxxcommand key2"));
    channel.CallMethod(NULL, &cntl, &request, &response, NULL);
    ASSERT_FALSE(cntl.Failed()) << cntl.ErrorText();
    ASSERT_EQ(7, response.reply_size());
    ASSERT_EQ(melon::REDIS_REPLY_NIL, response.reply(0).type());
    ASSERT_EQ(melon::REDIS_REPLY_NIL, response.reply(1).type());
    ASSERT_EQ(melon::REDIS_REPLY_STATUS, response.reply(2).type());
    ASSERT_STREQ("OK", response.reply(2).c_str());
    ASSERT_EQ(melon::REDIS_REPLY_STRING, response.reply(3).type());
    ASSERT_STREQ("value1", response.reply(3).c_str());
    ASSERT_EQ(melon::REDIS_REPLY_STATUS, response.reply(4).type());
    ASSERT_STREQ("OK", response.reply(4).c_str());
    ASSERT_EQ(melon::REDIS_REPLY_STRING, response.reply(5).type());
    ASSERT_STREQ("value2", response.reply(5).c_str());
    ASSERT_EQ(melon::REDIS_REPLY_ERROR, response.reply(6).type());
    ASSERT_TRUE(mutil::StringPiece(response.reply(6).error_message()).starts_with("ERR unknown command"));

    cntl.Reset(); 
    request.Clear();
    response.Clear();
    std::string value3("value3");
    value3.append(1, '\0');
    value3.append(1, 'a');
    std::vector<mutil::StringPiece> pieces;
    pieces.push_back("set");
    pieces.push_back("key3");
    pieces.push_back(value3);
    ASSERT_TRUE(request.AddCommandByComponents(&pieces[0], pieces.size()));
    ASSERT_TRUE(request.AddCommand("set key4 \"\""));
    ASSERT_TRUE(request.AddCommand("get key3"));
    ASSERT_TRUE(request.AddCommand("get key4"));
    channel.CallMethod(NULL, &cntl, &request, &response, NULL);
    ASSERT_FALSE(cntl.Failed()) << cntl.ErrorText();
    ASSERT_EQ(4, response.reply_size());
    ASSERT_EQ(melon::REDIS_REPLY_STATUS, response.reply(0).type());
    ASSERT_STREQ("OK", response.reply(0).c_str());
    ASSERT_EQ(melon::REDIS_REPLY_STATUS, response.reply(1).type());
    ASSERT_STREQ("OK", response.reply(1).c_str());
    ASSERT_EQ(melon::REDIS_REPLY_STRING, response.reply(2).type());
    ASSERT_STREQ("value3", response.reply(2).c_str());
    ASSERT_NE("value3", response.reply(2).data());
    ASSERT_EQ(value3, response.reply(2).data());
    ASSERT_EQ(melon::REDIS_REPLY_STRING, response.reply(3).type());
    ASSERT_EQ("", response.reply(3).data());
}

void* incr_thread(void* arg) {
    melon::Channel* c = static_cast<melon::Channel*>(arg);

    for (int i = 0; i < 5000; ++i) {
        melon::RedisRequest request;
        melon::RedisResponse response;
        melon::Controller cntl;
        EXPECT_TRUE(request.AddCommand("incr count"));
        c->CallMethod(NULL, &cntl, &request, &response, NULL);
        EXPECT_FALSE(cntl.Failed()) << cntl.ErrorText();
        EXPECT_EQ(1, response.reply_size());
        EXPECT_TRUE(response.reply(0).is_integer());
    }
    return NULL;
}

TEST_F(RedisTest, server_concurrency) {
    int N = 10;
    melon::Server server;
    melon::ServerOptions server_options;
    RedisServiceImpl* rsimpl = new RedisServiceImpl;
    IncrCommandHandler *ih = new IncrCommandHandler;
    rsimpl->AddCommandHandler("incr", ih);
    server_options.redis_service = rsimpl;
    melon::PortRange pr(8081, 8900);
    ASSERT_EQ(0, server.Start("0.0.0.0", pr, &server_options));

    melon::ChannelOptions options;
    options.protocol = melon::PROTOCOL_REDIS;
    options.connection_type = "pooled";
    std::vector<fiber_t> bths;
    std::vector<melon::Channel*> channels;
    for (int i = 0; i < N; ++i) {
        channels.push_back(new melon::Channel);
        ASSERT_EQ(0, channels.back()->Init("127.0.0.1", server.listen_address().port, &options));
        fiber_t bth;
        ASSERT_EQ(fiber_start_background(&bth, NULL, incr_thread, channels.back()), 0);
        bths.push_back(bth);
    }

    for (int i = 0; i < N; ++i) {
        fiber_join(bths[i], NULL);
        delete channels[i];
    }
    ASSERT_EQ(int_map["count"], 10 * 5000LL);
}

class MultiCommandHandler : public melon::RedisCommandHandler {
public:
    MultiCommandHandler() {}

    melon::RedisCommandHandlerResult Run(const std::vector<mutil::StringPiece>& args,
                                        melon::RedisReply* output,
                                        bool flush_batched) {
        output->SetStatus("OK");
        return melon::REDIS_CMD_CONTINUE;
    }

    RedisCommandHandler* NewTransactionHandler() override {
        return new MultiTransactionHandler;
    }

    class MultiTransactionHandler : public melon::RedisCommandHandler {
    public:
        melon::RedisCommandHandlerResult Run(const std::vector<mutil::StringPiece>& args,
                                            melon::RedisReply* output,
                                            bool flush_batched) {
            if (args[0] == "multi") {
                output->SetError("ERR duplicate multi");
                return melon::REDIS_CMD_CONTINUE;
            }
            if (args[0] != "exec") {
                std::vector<std::string> comm;
                for (int i = 0; i < (int)args.size(); ++i) {
                    comm.push_back(args[i].as_string());
                }
                _commands.push_back(comm);
                output->SetStatus("QUEUED");
                return melon::REDIS_CMD_CONTINUE;
            }
            output->SetArray(_commands.size());
            s_mutex.lock();
            for (size_t i = 0; i < _commands.size(); ++i) {
                if (_commands[i][0] == "incr") {
                    int64_t value;
                    value = ++int_map[_commands[i][1]];
                    (*output)[i].SetInteger(value);
                } else {
                    (*output)[i].SetStatus("unknown command");
                }
            }
            s_mutex.unlock();
            return melon::REDIS_CMD_HANDLED;
        }
    private:
        std::vector<std::vector<std::string> > _commands;
    };
};

TEST_F(RedisTest, server_command_continue) {
    melon::Server server;
    melon::ServerOptions server_options;
    RedisServiceImpl* rsimpl = new RedisServiceImpl;
    rsimpl->AddCommandHandler("get", new GetCommandHandler(rsimpl));
    rsimpl->AddCommandHandler("set", new SetCommandHandler(rsimpl));
    rsimpl->AddCommandHandler("incr", new IncrCommandHandler);
    rsimpl->AddCommandHandler("multi", new MultiCommandHandler);
    server_options.redis_service = rsimpl;
    melon::PortRange pr(8081, 8900);
    ASSERT_EQ(0, server.Start("127.0.0.1", pr, &server_options));

    melon::ChannelOptions options;
    options.protocol = melon::PROTOCOL_REDIS;
    melon::Channel channel;
    ASSERT_EQ(0, channel.Init("127.0.0.1", server.listen_address().port, &options));

    {
        melon::RedisRequest request;
        melon::RedisResponse response;
        melon::Controller cntl;
        ASSERT_TRUE(request.AddCommand("set hello world"));
        ASSERT_TRUE(request.AddCommand("get hello"));
        channel.CallMethod(NULL, &cntl, &request, &response, NULL);
        ASSERT_FALSE(cntl.Failed()) << cntl.ErrorText();
        ASSERT_EQ(2, response.reply_size());
        ASSERT_STREQ("world", response.reply(1).c_str());
    }
    {
        melon::RedisRequest request;
        melon::RedisResponse response;
        melon::Controller cntl;
        ASSERT_TRUE(request.AddCommand("multi"));
        ASSERT_TRUE(request.AddCommand("mUltI"));
        int count = 10;
        for (int i = 0; i < count; ++i) {
            ASSERT_TRUE(request.AddCommand("incr hello 1"));
        }
        ASSERT_TRUE(request.AddCommand("exec"));
        channel.CallMethod(NULL, &cntl, &request, &response, NULL);
        ASSERT_EQ(13, response.reply_size());
        ASSERT_FALSE(cntl.Failed()) << cntl.ErrorText();
        ASSERT_EQ(melon::REDIS_REPLY_STATUS, response.reply(0).type());
        ASSERT_STREQ("OK", response.reply(0).c_str());
        ASSERT_EQ(melon::REDIS_REPLY_ERROR, response.reply(1).type());
        for (int i = 2; i < count + 2; ++i) {
            ASSERT_EQ(melon::REDIS_REPLY_STATUS, response.reply(i).type());
            ASSERT_STREQ("QUEUED", response.reply(i).c_str());
        }
        const melon::RedisReply& m = response.reply(count + 2);
        ASSERT_EQ(count, (int)m.size());
        for (int i = 0; i < count; ++i) {
            ASSERT_EQ(i+1, m[i].integer());
        }
    }
    // After 'multi', normal requests should be successful
    {
        melon::RedisRequest request;
        melon::RedisResponse response;
        melon::Controller cntl;
        ASSERT_TRUE(request.AddCommand("get hello"));
        ASSERT_TRUE(request.AddCommand("get hello2"));
        ASSERT_TRUE(request.AddCommand("set key1 value1"));
        ASSERT_TRUE(request.AddCommand("get key1"));
        channel.CallMethod(NULL, &cntl, &request, &response, NULL);
        ASSERT_FALSE(cntl.Failed()) << cntl.ErrorText();
        ASSERT_STREQ("world", response.reply(0).c_str());
        ASSERT_EQ(melon::REDIS_REPLY_NIL, response.reply(1).type());
        ASSERT_EQ(melon::REDIS_REPLY_STATUS, response.reply(2).type());
        ASSERT_STREQ("OK", response.reply(2).c_str());
        ASSERT_EQ(melon::REDIS_REPLY_STRING, response.reply(3).type());
        ASSERT_STREQ("value1", response.reply(3).c_str());
    }
}

TEST_F(RedisTest, server_handle_pipeline) {
    melon::Server server;
    melon::ServerOptions server_options;
    RedisServiceImpl* rsimpl = new RedisServiceImpl;
    GetCommandHandler* getch = new GetCommandHandler(rsimpl, true);
    SetCommandHandler* setch = new SetCommandHandler(rsimpl, true);
    rsimpl->AddCommandHandler("get", getch);
    rsimpl->AddCommandHandler("set", setch);
    rsimpl->AddCommandHandler("multi", new MultiCommandHandler);
    server_options.redis_service = rsimpl;
    melon::PortRange pr(8081, 8900);
    ASSERT_EQ(0, server.Start("127.0.0.1", pr, &server_options));

    melon::ChannelOptions options;
    options.protocol = melon::PROTOCOL_REDIS;
    melon::Channel channel;
    ASSERT_EQ(0, channel.Init("127.0.0.1", server.listen_address().port, &options));

    melon::RedisRequest request;
    melon::RedisResponse response;
    melon::Controller cntl;
    ASSERT_TRUE(request.AddCommand("set key1 v1"));
    ASSERT_TRUE(request.AddCommand("set key2 v2"));
    ASSERT_TRUE(request.AddCommand("set key3 v3"));
    ASSERT_TRUE(request.AddCommand("get hello"));
    ASSERT_TRUE(request.AddCommand("get hello"));
    ASSERT_TRUE(request.AddCommand("set key1 world"));
    ASSERT_TRUE(request.AddCommand("set key2 world"));
    ASSERT_TRUE(request.AddCommand("get key2"));
    channel.CallMethod(NULL, &cntl, &request, &response, NULL);
    ASSERT_FALSE(cntl.Failed()) << cntl.ErrorText();
    ASSERT_EQ(8, response.reply_size());
    ASSERT_EQ(1, rsimpl->_batch_count);
    ASSERT_TRUE(response.reply(7).is_string());
    ASSERT_STREQ(response.reply(7).c_str(), "world");
}

} //namespace
