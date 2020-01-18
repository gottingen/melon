//

// This test serves primarily as a compilation test for base/raw_logging.h.
// Raw logging testing is covered by logging_unittest.cc, which is not as
// portable as this test.

#include <abel/base/internal/raw_logging.h>

#include <tuple>

#include <gtest/gtest.h>
#include <abel/strings/str_cat.h>

namespace {

TEST(RawLoggingCompilationTest, Log) {
  ABEL_RAW_LOG(INFO, "RAW INFO: %d", 1);
  ABEL_RAW_LOG(INFO, "RAW INFO: %d %d", 1, 2);
  ABEL_RAW_LOG(INFO, "RAW INFO: %d %d %d", 1, 2, 3);
  ABEL_RAW_LOG(INFO, "RAW INFO: %d %d %d %d", 1, 2, 3, 4);
  ABEL_RAW_LOG(INFO, "RAW INFO: %d %d %d %d %d", 1, 2, 3, 4, 5);
  ABEL_RAW_LOG(WARNING, "RAW WARNING: %d", 1);
  ABEL_RAW_LOG(ERROR, "RAW ERROR: %d", 1);
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
  EXPECT_DEATH_IF_SUPPORTED(ABEL_RAW_LOG(FATAL, "my dog has fleas"),
                            kExpectedDeathOutput);
}

TEST(InternalLog, CompilationTest) {
  ABEL_INTERNAL_LOG(INFO, "Internal Log");
  std::string log_msg = "Internal Log";
  ABEL_INTERNAL_LOG(INFO, log_msg);

  ABEL_INTERNAL_LOG(INFO, log_msg + " 2");

  float d = 1.1f;
  ABEL_INTERNAL_LOG(INFO, abel::string_cat("Internal log ", 3, " + ", d));
}

TEST(InternalLogDeathTest, FailingCheck) {
  EXPECT_DEATH_IF_SUPPORTED(ABEL_INTERNAL_CHECK(1 == 0, "explanation"),
                            kExpectedDeathOutput);
}

TEST(InternalLogDeathTest, LogFatal) {
  EXPECT_DEATH_IF_SUPPORTED(ABEL_INTERNAL_LOG(FATAL, "my dog has fleas"),
                            kExpectedDeathOutput);
}

}  // namespace
