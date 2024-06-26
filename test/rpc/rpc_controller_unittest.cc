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




// Date: Sun Jul 13 15:04:18 CST 2014

#include <gtest/gtest.h>
#include <google/protobuf/stubs/common.h>
#include <turbo/log/logging.h>
#include <melon/utility/time.h>
#include <melon/utility/macros.h>
#include <melon/rpc/socket.h>
#include <melon/rpc/server.h>
#include <melon/rpc/channel.h>
#include <melon/rpc/controller.h>
#include <melon/utility/config.h>

class ControllerTest : public ::testing::Test{
protected:
    ControllerTest() {};
    virtual ~ControllerTest(){};
    virtual void SetUp() {};
    virtual void TearDown() {};
};

void MyCancelCallback(bool* cancel_flag) {
    *cancel_flag = true;
}

TEST_F(ControllerTest, notify_on_failed) {
    melon::SocketId id = 0;
    ASSERT_EQ(0, melon::Socket::Create(melon::SocketOptions(), &id));

    melon::Controller cntl;
    cntl._current_call.peer_id = id;
    ASSERT_FALSE(cntl.IsCanceled());

    bool cancel = false;
    cntl.NotifyOnCancel(melon::NewCallback(&MyCancelCallback, &cancel));
    // Trigger callback
    melon::Socket::SetFailed(id);
    usleep(20000); // sleep a while to wait for the canceling which will be
                   // happening in another thread.
    ASSERT_TRUE(cancel);
    ASSERT_TRUE(cntl.IsCanceled());
}

TEST_F(ControllerTest, notify_on_destruction) {
    melon::SocketId id = 0;
    ASSERT_EQ(0, melon::Socket::Create(melon::SocketOptions(), &id));

    melon::Controller* cntl = new melon::Controller;
    cntl->_current_call.peer_id = id;
    ASSERT_FALSE(cntl->IsCanceled());

    bool cancel = false;
    cntl->NotifyOnCancel(melon::NewCallback(&MyCancelCallback, &cancel));
    // Trigger callback
    delete cntl;
    ASSERT_TRUE(cancel);
}

#if ! MELON_WITH_GLOG

static bool endsWith(const std::string& s1, const mutil::StringPiece& s2)  {
    if (s1.size() < s2.size()) {
        return false;
    }
    return memcmp(s1.data() + s1.size() - s2.size(), s2.data(), s2.size()) == 0;
}
static bool startsWith(const std::string& s1, const mutil::StringPiece& s2)  {
    if (s1.size() < s2.size()) {
        return false;
    }
    return memcmp(s1.data(), s2.data(), s2.size()) == 0;
}

DECLARE_bool(log_as_json);

TEST_F(ControllerTest, SessionKV) {
    FLAGS_log_as_json = false;
    logging::StringSink sink1;
    auto oldSink = logging::SetLogSink(&sink1);
    {
        melon::Controller cntl;
        cntl.set_log_id(123); // not working now
        // set
        cntl.SessionKV().Set("Apple", 1234567);    
        cntl.SessionKV().Set("Baidu", "Building");
        // get
        auto v1 = cntl.SessionKV().Get("Apple");
        ASSERT_TRUE(v1);
        ASSERT_EQ("1234567", *v1);
        auto v2 = cntl.SessionKV().Get("Baidu");
        ASSERT_TRUE(v2);
        ASSERT_EQ("Building", *v2);

        // override
        cntl.SessionKV().Set("Baidu", "NewStuff");
        v2 = cntl.SessionKV().Get("Baidu");
        ASSERT_TRUE(v2);
        ASSERT_EQ("NewStuff", *v2);

        cntl.SessionKV().Set("Cisco", 33.33);

        CLOGW(&cntl) << "My WARNING Log";
        ASSERT_TRUE(endsWith(sink1, "] My WARNING Log")) << sink1;
        ASSERT_TRUE(startsWith(sink1, "W")) << sink1;
        sink1.clear();

        cntl.set_request_id("abcdEFG-456");
        CLOGE(&cntl) << "My ERROR Log";
        ASSERT_TRUE(endsWith(sink1, "] @rid=abcdEFG-456 My ERROR Log")) << sink1;
        ASSERT_TRUE(startsWith(sink1, "E")) << sink1;
        sink1.clear();

        FLAGS_log_as_json = true;
    }
    ASSERT_TRUE(endsWith(sink1, R"(,"@rid":"abcdEFG-456","M":"Session ends.","Baidu":"NewStuff","Cisco":"33.330000","Apple":"1234567"})")) << sink1;
    ASSERT_TRUE(startsWith(sink1, R"({"L":"I",)")) << sink1;

    logging::SetLogSink(oldSink);
}
#endif
