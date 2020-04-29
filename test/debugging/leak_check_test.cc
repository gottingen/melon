//

#include <string>

#include <gtest/gtest.h>
#include <abel/log/abel_logging.h>
#include <abel/debugging/leak_check.h>

namespace {

    TEST(LeakCheckTest, DetectLeakSanitizer) {
#ifdef ABEL_EXPECT_LEAK_SANITIZER
        EXPECT_TRUE(abel::have_leak_sanitizer());
#else
        EXPECT_FALSE(abel::have_leak_sanitizer());
#endif
    }

    TEST(LeakCheckTest, IgnoreLeakSuppressesLeakedMemoryErrors) {
        auto foo = abel::ignore_leak(new std::string("some ignored leaked string"));
        ABEL_RAW_INFO("Ignoring leaked std::string {}", foo->c_str());
    }

    TEST(LeakCheckTest, LeakCheckDisablerIgnoresLeak) {
        abel::leak_check_disabler disabler;
        auto foo = new std::string("some std::string leaked while checks are disabled");
        ABEL_RAW_INFO("Ignoring leaked std::string {}", foo->c_str());
    }

}  // namespace
