
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/

#include "melon/bootstrap/flags.h"
#include "melon/bootstrap/bootstrap.h"
#include "gflags/gflags.h"
#include "testing/gtest_wrap.h"

DEFINE_bool(test, true, "");

MELON_RESET_FLAGS(test, false);

namespace melon {

    TEST(OverrideFlag, All) { EXPECT_FALSE(FLAGS_test); }

}  // namespace melon::init

int main(int argc, char **argv) {
    melon::bootstrap_init(argc, argv);
    return ::RUN_ALL_TESTS();
}
