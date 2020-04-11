//

// This test serves primarily as a compilation test for base/raw_logging.h.
// Raw logging testing is covered by logging_unittest.cc, which is not as
// portable as this test.

#include <abel/log/abel_logging.h>
#include <tuple>
#include <gtest/gtest.h>
#include <abel/strings/str_cat.h>

namespace {

    TEST(RawLoggingCompilationTest, Log) {
        ABEL_RAW_INFO( "RAW INFO: {}", 1);
        ABEL_RAW_INFO("RAW INFO: {} {}", 1, 2);
        ABEL_RAW_INFO("RAW INFO: {} {} {}", 1, 2, 3);
        ABEL_RAW_INFO("RAW INFO: {} {} {} {}", 1, 2, 3, 4);
        ABEL_RAW_INFO("RAW INFO:{} {} {} {} {}", 1, 2, 3, 4, 5);
        ABEL_RAW_WARN("RAW WARNING: {}", 1);
        ABEL_RAW_ERROR("RAW ERROR: {}", 1);
    }

    TEST(RawLoggingCompilationTest, PassingCheck) {
        ABEL_RAW_CHECK(true, "RAW CHECK");
    }

// Not all platforms support output from raw log, so we don't verify any
// particular output for RAW check failures (expecting the empty string
// accomplishes this).  This test is primarily a compilation test, but we
// are verifying process death when EXPECT_DEATH works for a platform.
    const char kExpectedDeathOutput[] = "";

    TEST(RawLoggingDeathTest, FailingCheck) {
        EXPECT_DEATH_IF_SUPPORTED(ABEL_RAW_CHECK(1 == 0, "explanation"),
                                  kExpectedDeathOutput);
    }

    TEST(RawLoggingDeathTest, LogFatal) {
        EXPECT_DEATH_IF_SUPPORTED(ABEL_RAW_CRITICAL("my dog has fleas"),
                                  kExpectedDeathOutput);
    }

    TEST(InternalLog, CompilationTest) {
        ABEL_RAW_INFO("Internal Log");
        std::string log_msg = "Internal Log";
        ABEL_RAW_INFO(log_msg);

        ABEL_RAW_INFO(log_msg + " 2");

        float d = 1.1f;
        ABEL_RAW_INFO(abel::string_cat("Internal log ", 3, " + ", d));
    }

    TEST(InternalLogDeathTest, FailingCheck) {
        EXPECT_DEATH_IF_SUPPORTED(ABEL_RAW_CHECK(1 == 0, "explanation"),
                                  kExpectedDeathOutput);
    }

    TEST(InternalLogDeathTest, LogFatal) {
        EXPECT_DEATH_IF_SUPPORTED(ABEL_RAW_CRITICAL("my dog has fleas"),
                                  kExpectedDeathOutput);
    }

}  // namespace
