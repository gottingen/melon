//

#include <abel/format/internal/output.h>
#include <sstream>
#include <string>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace abel {

namespace {

TEST(invoke_flush, string) {
    std::string str = "ABC";
    format_internal::invoke_flush(&str, "DEF");
    EXPECT_EQ(str, "ABCDEF");
}

TEST(invoke_flush, stream) {
    std::stringstream str;
    str << "ABC";
    format_internal::invoke_flush(&str, "DEF");
    EXPECT_EQ(str.str(), "ABCDEF");
}

TEST(buffer_raw_sink, Limits) {
    char buf[16];
    {
        std::fill(std::begin(buf), std::end(buf), 'x');
        format_internal::buffer_raw_sink bufsink(buf, sizeof(buf) - 1);
        format_internal::invoke_flush(&bufsink, "Hello World237");
        EXPECT_EQ(std::string(buf, sizeof(buf)), "Hello World237xx");
    }
    {
        std::fill(std::begin(buf), std::end(buf), 'x');
        format_internal::buffer_raw_sink bufsink(buf, sizeof(buf) - 1);
        format_internal::invoke_flush(&bufsink, "Hello World237237");
        EXPECT_EQ(std::string(buf, sizeof(buf)), "Hello World2372x");
    }
    {
        std::fill(std::begin(buf), std::end(buf), 'x');
        format_internal::buffer_raw_sink bufsink(buf, sizeof(buf) - 1);
        format_internal::invoke_flush(&bufsink, "Hello World");
        format_internal::invoke_flush(&bufsink, "237");
        EXPECT_EQ(std::string(buf, sizeof(buf)), "Hello World237xx");
    }
    {
        std::fill(std::begin(buf), std::end(buf), 'x');
        format_internal::buffer_raw_sink bufsink(buf, sizeof(buf) - 1);
        format_internal::invoke_flush(&bufsink, "Hello World");
        format_internal::invoke_flush(&bufsink, "237237");
        EXPECT_EQ(std::string(buf, sizeof(buf)), "Hello World2372x");
    }
}

}  // namespace

}  // namespace abel

