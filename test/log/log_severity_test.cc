//

#include <abel/log/log_severity.h>
#include <cstdint>
#include <ios>
#include <limits>
#include <ostream>
#include <sstream>
#include <string>
#include <tuple>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <abel/config/flags/marshalling.h>
#include <abel/strings/str_cat.h>

namespace {
    using ::testing::Eq;
    using ::testing::IsFalse;
    using ::testing::IsTrue;
    using ::testing::TestWithParam;
    using ::testing::Values;

    std::string StreamHelper(abel::LogSeverity value) {
        std::ostringstream stream;
        stream << value;
        return stream.str();
    }

    TEST(StreamTest, Works) {
        EXPECT_THAT(StreamHelper(static_cast<abel::LogSeverity>(-100)),
                    Eq("abel::LogSeverity(-100)"));
        EXPECT_THAT(StreamHelper(abel::LogSeverity::kInfo), Eq("INFO"));
        EXPECT_THAT(StreamHelper(abel::LogSeverity::kWarning), Eq("WARNING"));
        EXPECT_THAT(StreamHelper(abel::LogSeverity::kError), Eq("ERROR"));
        EXPECT_THAT(StreamHelper(abel::LogSeverity::kFatal), Eq("FATAL"));
        EXPECT_THAT(StreamHelper(static_cast<abel::LogSeverity>(4)),
                    Eq("abel::LogSeverity(4)"));
    }

    using ParseFlagFromOutOfRangeIntegerTest = TestWithParam<int64_t>;

    INSTANTIATE_TEST_SUITE_P(
            Instantiation, ParseFlagFromOutOfRangeIntegerTest,
            Values(static_cast<int64_t>(std::numeric_limits<int>::min()) - 1,
                   static_cast<int64_t>(std::numeric_limits<int>::max()) + 1));

    TEST_P(ParseFlagFromOutOfRangeIntegerTest, ReturnsError) {
        const std::string to_parse = abel::string_cat(GetParam());
        abel::LogSeverity value;
        std::string error;
        EXPECT_THAT(abel::ParseFlag(to_parse, &value, &error), IsFalse()) << value;
    }

    using ParseFlagFromAlmostOutOfRangeIntegerTest = TestWithParam<int>;

    INSTANTIATE_TEST_SUITE_P(Instantiation,
                             ParseFlagFromAlmostOutOfRangeIntegerTest,
                             Values(std::numeric_limits<int>::min(),
                                    std::numeric_limits<int>::max()));

    TEST_P(ParseFlagFromAlmostOutOfRangeIntegerTest, YieldsExpectedValue) {
        const auto expected = static_cast<abel::LogSeverity>(GetParam());
        const std::string to_parse = abel::string_cat(GetParam());
        abel::LogSeverity value;
        std::string error;
        ASSERT_THAT(abel::ParseFlag(to_parse, &value, &error), IsTrue()) << error;
        EXPECT_THAT(value, Eq(expected));
    }

    using ParseFlagFromIntegerMatchingEnumeratorTest =
    TestWithParam<std::tuple<abel::string_view, abel::LogSeverity>>;

    INSTANTIATE_TEST_SUITE_P(
            Instantiation, ParseFlagFromIntegerMatchingEnumeratorTest,
            Values(std::make_tuple("0", abel::LogSeverity::kInfo),
                   std::make_tuple(" 0", abel::LogSeverity::kInfo),
                   std::make_tuple("-0", abel::LogSeverity::kInfo),
                   std::make_tuple("+0", abel::LogSeverity::kInfo),
                   std::make_tuple("00", abel::LogSeverity::kInfo),
                   std::make_tuple("0 ", abel::LogSeverity::kInfo),
                   std::make_tuple("0x0", abel::LogSeverity::kInfo),
                   std::make_tuple("1", abel::LogSeverity::kWarning),
                   std::make_tuple("+1", abel::LogSeverity::kWarning),
                   std::make_tuple("2", abel::LogSeverity::kError),
                   std::make_tuple("3", abel::LogSeverity::kFatal)));

    TEST_P(ParseFlagFromIntegerMatchingEnumeratorTest, YieldsExpectedValue) {
        const abel::string_view to_parse = std::get<0>(GetParam());
        const abel::LogSeverity expected = std::get<1>(GetParam());
        abel::LogSeverity value;
        std::string error;
        ASSERT_THAT(abel::ParseFlag(to_parse, &value, &error), IsTrue()) << error;
        EXPECT_THAT(value, Eq(expected));
    }

    using ParseFlagFromOtherIntegerTest =
    TestWithParam<std::tuple<abel::string_view, int>>;

    INSTANTIATE_TEST_SUITE_P(Instantiation, ParseFlagFromOtherIntegerTest,
                             Values(std::make_tuple("-1", -1),
                                    std::make_tuple("4", 4),
                                    std::make_tuple("010", 10),
                                    std::make_tuple("0x10", 16)));

    TEST_P(ParseFlagFromOtherIntegerTest, YieldsExpectedValue) {
        const abel::string_view to_parse = std::get<0>(GetParam());
        const auto expected = static_cast<abel::LogSeverity>(std::get<1>(GetParam()));
        abel::LogSeverity value;
        std::string error;
        ASSERT_THAT(abel::ParseFlag(to_parse, &value, &error), IsTrue()) << error;
        EXPECT_THAT(value, Eq(expected));
    }

    using ParseFlagFromEnumeratorTest =
    TestWithParam<std::tuple<abel::string_view, abel::LogSeverity>>;

    INSTANTIATE_TEST_SUITE_P(
            Instantiation, ParseFlagFromEnumeratorTest,
            Values(std::make_tuple("INFO", abel::LogSeverity::kInfo),
                   std::make_tuple("info", abel::LogSeverity::kInfo),
                   std::make_tuple("kInfo", abel::LogSeverity::kInfo),
                   std::make_tuple("iNfO", abel::LogSeverity::kInfo),
                   std::make_tuple("kInFo", abel::LogSeverity::kInfo),
                   std::make_tuple("WARNING", abel::LogSeverity::kWarning),
                   std::make_tuple("warning", abel::LogSeverity::kWarning),
                   std::make_tuple("kWarning", abel::LogSeverity::kWarning),
                   std::make_tuple("WaRnInG", abel::LogSeverity::kWarning),
                   std::make_tuple("KwArNiNg", abel::LogSeverity::kWarning),
                   std::make_tuple("ERROR", abel::LogSeverity::kError),
                   std::make_tuple("error", abel::LogSeverity::kError),
                   std::make_tuple("kError", abel::LogSeverity::kError),
                   std::make_tuple("eRrOr", abel::LogSeverity::kError),
                   std::make_tuple("kErRoR", abel::LogSeverity::kError),
                   std::make_tuple("FATAL", abel::LogSeverity::kFatal),
                   std::make_tuple("fatal", abel::LogSeverity::kFatal),
                   std::make_tuple("kFatal", abel::LogSeverity::kFatal),
                   std::make_tuple("FaTaL", abel::LogSeverity::kFatal),
                   std::make_tuple("KfAtAl", abel::LogSeverity::kFatal)));

    TEST_P(ParseFlagFromEnumeratorTest, YieldsExpectedValue) {
        const abel::string_view to_parse = std::get<0>(GetParam());
        const abel::LogSeverity expected = std::get<1>(GetParam());
        abel::LogSeverity value;
        std::string error;
        ASSERT_THAT(abel::ParseFlag(to_parse, &value, &error), IsTrue()) << error;
        EXPECT_THAT(value, Eq(expected));
    }

    using ParseFlagFromGarbageTest = TestWithParam<abel::string_view>;

    INSTANTIATE_TEST_SUITE_P(Instantiation, ParseFlagFromGarbageTest,
                             Values("", "\0", " ", "garbage", "kkinfo", "I"));

    TEST_P(ParseFlagFromGarbageTest, ReturnsError) {
        const abel::string_view to_parse = GetParam();
        abel::LogSeverity value;
        std::string error;
        EXPECT_THAT(abel::ParseFlag(to_parse, &value, &error), IsFalse()) << value;
    }

    using UnparseFlagToEnumeratorTest =
    TestWithParam<std::tuple<abel::LogSeverity, abel::string_view>>;

    INSTANTIATE_TEST_SUITE_P(
            Instantiation, UnparseFlagToEnumeratorTest,
            Values(std::make_tuple(abel::LogSeverity::kInfo, "INFO"),
                   std::make_tuple(abel::LogSeverity::kWarning, "WARNING"),
                   std::make_tuple(abel::LogSeverity::kError, "ERROR"),
                   std::make_tuple(abel::LogSeverity::kFatal, "FATAL")));

    TEST_P(UnparseFlagToEnumeratorTest, ReturnsExpectedValueAndRoundTrips) {
        const abel::LogSeverity to_unparse = std::get<0>(GetParam());
        const abel::string_view expected = std::get<1>(GetParam());
        const std::string stringified_value = abel::UnparseFlag(to_unparse);
        EXPECT_THAT(stringified_value, Eq(expected));
        abel::LogSeverity reparsed_value;
        std::string error;
        EXPECT_THAT(abel::ParseFlag(stringified_value, &reparsed_value, &error),
                    IsTrue());
        EXPECT_THAT(reparsed_value, Eq(to_unparse));
    }

    using UnparseFlagToOtherIntegerTest = TestWithParam<int>;

    INSTANTIATE_TEST_SUITE_P(Instantiation, UnparseFlagToOtherIntegerTest,
                             Values(std::numeric_limits<int>::min(), -1, 4,
                                    std::numeric_limits<int>::max()));

    TEST_P(UnparseFlagToOtherIntegerTest, ReturnsExpectedValueAndRoundTrips) {
        const abel::LogSeverity to_unparse =
                static_cast<abel::LogSeverity>(GetParam());
        const std::string expected = abel::string_cat(GetParam());
        const std::string stringified_value = abel::UnparseFlag(to_unparse);
        EXPECT_THAT(stringified_value, Eq(expected));
        abel::LogSeverity reparsed_value;
        std::string error;
        EXPECT_THAT(abel::ParseFlag(stringified_value, &reparsed_value, &error),
                    IsTrue());
        EXPECT_THAT(reparsed_value, Eq(to_unparse));
    }
}  // namespace
