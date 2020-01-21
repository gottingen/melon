//

#include <abel/strings/internal/str_format/output.h>

#include <sstream>
#include <string>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace abel {

namespace {

TEST(InvokeFlush, String) {
  std::string str = "ABC";
  str_format_internal::InvokeFlush(&str, "DEF");
  EXPECT_EQ(str, "ABCDEF");
}

TEST(InvokeFlush, Stream) {
  std::stringstream str;
  str << "ABC";
  str_format_internal::InvokeFlush(&str, "DEF");
  EXPECT_EQ(str.str(), "ABCDEF");
}

TEST(BufferRawSink, Limits) {
  char buf[16];
  {
    std::fill(std::begin(buf), std::end(buf), 'x');
    str_format_internal::BufferRawSink bufsink(buf, sizeof(buf) - 1);
    str_format_internal::InvokeFlush(&bufsink, "Hello World237");
    EXPECT_EQ(std::string(buf, sizeof(buf)), "Hello World237xx");
  }
  {
    std::fill(std::begin(buf), std::end(buf), 'x');
    str_format_internal::BufferRawSink bufsink(buf, sizeof(buf) - 1);
    str_format_internal::InvokeFlush(&bufsink, "Hello World237237");
    EXPECT_EQ(std::string(buf, sizeof(buf)), "Hello World2372x");
  }
  {
    std::fill(std::begin(buf), std::end(buf), 'x');
    str_format_internal::BufferRawSink bufsink(buf, sizeof(buf) - 1);
    str_format_internal::InvokeFlush(&bufsink, "Hello World");
    str_format_internal::InvokeFlush(&bufsink, "237");
    EXPECT_EQ(std::string(buf, sizeof(buf)), "Hello World237xx");
  }
  {
    std::fill(std::begin(buf), std::end(buf), 'x');
    str_format_internal::BufferRawSink bufsink(buf, sizeof(buf) - 1);
    str_format_internal::InvokeFlush(&bufsink, "Hello World");
    str_format_internal::InvokeFlush(&bufsink, "237237");
    EXPECT_EQ(std::string(buf, sizeof(buf)), "Hello World2372x");
  }
}

}  // namespace

}  // namespace abel

