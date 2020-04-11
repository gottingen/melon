//

#include <abel/log/log.h>
#include <cstdint>
#include <ios>
#include <limits>
#include <ostream>
#include <sstream>
#include <string>
#include <tuple>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <abel/config/marshalling.h>
#include <abel/strings/str_cat.h>
#include <abel/config/flag.h>

namespace {
    using ::testing::Eq;
    using ::testing::IsFalse;
    using ::testing::IsTrue;
    using ::testing::TestWithParam;
    using ::testing::Values;

    std::string StreamHelper(abel::log::level_enum value) {
        std::ostringstream stream;
        stream << abel::log::to_c_str(value);
        return stream.str();
    }

    TEST(StreamTest, Works) {
        EXPECT_THAT(StreamHelper(abel::log::level_enum::info), Eq("info"));
        EXPECT_THAT(StreamHelper(abel::log::level_enum::warn), Eq("warning"));
        EXPECT_THAT(StreamHelper(abel::log::level_enum::err), Eq("error"));
        EXPECT_THAT(StreamHelper(abel::log::level_enum::critical), Eq("critical"));
    }
}  // namespace
