// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "melon/utility/callback_helpers.h"

#include "melon/utility/bind.h"
#include "melon/utility/callback.h"
#include <gtest/gtest.h>

namespace {

void Increment(int* value) {
  (*value)++;
}

TEST(BindHelpersTest, TestScopedClosureRunnerExitScope) {
  int run_count = 0;
  {
    mutil::ScopedClosureRunner runner(mutil::Bind(&Increment, &run_count));
    EXPECT_EQ(0, run_count);
  }
  EXPECT_EQ(1, run_count);
}

TEST(BindHelpersTest, TestScopedClosureRunnerRelease) {
  int run_count = 0;
  mutil::Closure c;
  {
    mutil::ScopedClosureRunner runner(mutil::Bind(&Increment, &run_count));
    c = runner.Release();
    EXPECT_EQ(0, run_count);
  }
  EXPECT_EQ(0, run_count);
  c.Run();
  EXPECT_EQ(1, run_count);
}

TEST(BindHelpersTest, TestScopedClosureRunnerReset) {
  int run_count_1 = 0;
  int run_count_2 = 0;
  {
    mutil::ScopedClosureRunner runner;
    runner.Reset(mutil::Bind(&Increment, &run_count_1));
    runner.Reset(mutil::Bind(&Increment, &run_count_2));
    EXPECT_EQ(1, run_count_1);
    EXPECT_EQ(0, run_count_2);
  }
  EXPECT_EQ(1, run_count_2);

  int run_count_3 = 0;
  {
    mutil::ScopedClosureRunner runner(mutil::Bind(&Increment, &run_count_3));
    EXPECT_EQ(0, run_count_3);
    runner.Reset();
    EXPECT_EQ(1, run_count_3);
  }
  EXPECT_EQ(1, run_count_3);
}

}  // namespace
