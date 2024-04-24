// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "melon/utility/threading/thread_id_name_manager.h"

#include "melon/utility/threading/platform_thread.h"
#include <gtest/gtest.h>

typedef testing::Test ThreadIdNameManagerTest;

namespace {

TEST_F(ThreadIdNameManagerTest, ThreadNameInterning) {
  mutil::ThreadIdNameManager* manager = mutil::ThreadIdNameManager::GetInstance();

  mutil::PlatformThreadId a_id = mutil::PlatformThread::CurrentId();
  mutil::PlatformThread::SetName("First Name");
  std::string version = manager->GetName(a_id);

  mutil::PlatformThread::SetName("New name");
  EXPECT_NE(version, manager->GetName(a_id));
  mutil::PlatformThread::SetName("");
}

TEST_F(ThreadIdNameManagerTest, ResettingNameKeepsCorrectInternedValue) {
  mutil::ThreadIdNameManager* manager = mutil::ThreadIdNameManager::GetInstance();

  mutil::PlatformThreadId a_id = mutil::PlatformThread::CurrentId();
  mutil::PlatformThread::SetName("Test Name");
  std::string version = manager->GetName(a_id);

  mutil::PlatformThread::SetName("New name");
  EXPECT_NE(version, manager->GetName(a_id));

  mutil::PlatformThread::SetName("Test Name");
  EXPECT_EQ(version, manager->GetName(a_id));

  mutil::PlatformThread::SetName("");
}

}  // namespace
