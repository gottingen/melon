// Licensed to the Apache Software Foundation (ASF) under one
// or more contributor license agreements.  See the NOTICE file
// distributed with this work for additional information
// regarding copyright ownership.  The ASF licenses this file
// to you under the Apache License, Version 2.0 (the
// "License"); you may not use this file except in compliance
// with the License.  You may obtain a copy of the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing,
// software distributed under the License is distributed on an
// "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied.  See the License for the
// specific language governing permissions and limitations
// under the License.

// brpc - A framework to host and access services throughout Baidu.

// Date: Sun Jul 13 15:04:18 CST 2014

#include <gtest/gtest.h>
#include <google/protobuf/stubs/common.h>
#include "melon/common/logging.h"
#include "melon/butil/time.h"
#include "melon/butil/macros.h"
#include "melon/rpc/socket.h"
#include "melon/rpc/server.h"
#include "melon/rpc/channel.h"
#include "melon/rpc/controller.h"

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

