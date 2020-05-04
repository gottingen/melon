//

#include <memory>
#include <gtest/gtest.h>
#include <abel/log/abel_logging.h>
#include <abel/debugging/leak_check.h>

namespace {

    TEST(LeakCheckTest, LeakMemory) {
        // This test is expected to cause lsan failures on program exit. Therefore the
        // test will be run only by leak_check_test.sh, which will verify a
        // failed exit code.

        char *foo = strdup("lsan should complain about this leaked string");
        ABEL_RAW_INFO("Should detect leaked std::string {}", foo);
    }

    TEST(LeakCheckTest, LeakMemoryAfterDisablerScope) {
        // This test is expected to cause lsan failures on program exit. Therefore the
        // test will be run only by external_leak_check_test.sh, which will verify a
        // failed exit code.
        { abel::leak_check_disabler disabler; }
        char *foo = strdup("lsan should also complain about this leaked string");
        ABEL_RAW_INFO("Re-enabled leak detection.Should detect leaked std::string {}",
                     foo);
    }

}  // namespace
