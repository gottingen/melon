// Copyright (c) 2021, gottingen group.
// All rights reserved.
// Created by liyinbin lijippy@163.com


#include "abel/status/status.h"
#include <utility>
#include "gtest/gtest.h"

using abel::status;

TEST(status, MoveConstructor) {
    {
        status ok = status::ok();
        status ok2 = std::move(ok);

        ASSERT_TRUE(ok2.is_ok());
    }

    {
        status status1 = status::not_found("custom NotFound status message");
        status status2 = std::move(status1);

        ASSERT_TRUE(status2.is_not_found());
        ASSERT_EQ("NotFound: custom NotFound status message", status2.to_string());
    }

    {
        status self_moved = status::io_error("custom IOError status message");

        // Needed to bypass compiler warning about explicit move-assignment.
        status &self_moved_reference = self_moved;
        self_moved_reference = std::move(self_moved);
    }
}

