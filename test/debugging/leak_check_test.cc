
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/
#include <string>

#include "testing/gtest_wrap.h"
#include "melon/log/logging.h"
#include "melon/debugging/leak_check.h"

namespace {

    TEST(LeakCheckTest, DetectLeakSanitizer) {
#ifdef MELON_EXPECT_LEAK_SANITIZER
        EXPECT_TRUE(melon::debugging::have_leak_sanitizer());
#else
        EXPECT_FALSE(melon::debugging::have_leak_sanitizer());
#endif
    }

    TEST(LeakCheckTest, IgnoreLeakSuppressesLeakedMemoryErrors) {
        auto foo = melon::debugging::ignore_leak(new std::string("some ignored leaked string"));
        MELON_LOG(INFO)<<"Ignoring leaked std::string "<< foo->c_str();
    }

    TEST(LeakCheckTest, LeakCheckDisablerIgnoresLeak) {
        melon::debugging::leak_check_disabler disabler;
        auto foo = new std::string("some std::string leaked while checks are disabled");
        MELON_LOG(INFO)<<"Ignoring leaked std::string "<< foo->c_str();
    }

}  // namespace
