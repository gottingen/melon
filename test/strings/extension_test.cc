//
//

#include <abel/strings/internal/str_format/extension.h>

#include <random>
#include <string>

#include <abel/strings/str_format.h>

#include <gtest/gtest.h>

namespace {

std::string MakeRandomString(size_t len) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<> dis('a', 'z');
  std::string s(len, '0');
  for (char& c : s) {
    c = dis(gen);
  }
  return s;
}

TEST(FormatExtensionTest, SinkAppendSubstring) {
  for (size_t chunk_size : {1, 10, 100, 1000, 10000}) {
    std::string expected, actual;
    abel::str_format_internal::FormatSinkImpl sink(&actual);
    for (size_t chunks = 0; chunks < 10; ++chunks) {
      std::string rand = MakeRandomString(chunk_size);
      expected += rand;
      sink.Append(rand);
    }
    sink.Flush();
    EXPECT_EQ(actual, expected);
  }
}

TEST(FormatExtensionTest, SinkAppendChars) {
  for (size_t chunk_size : {1, 10, 100, 1000, 10000}) {
    std::string expected, actual;
    abel::str_format_internal::FormatSinkImpl sink(&actual);
    for (size_t chunks = 0; chunks < 10; ++chunks) {
      std::string rand = MakeRandomString(1);
      expected.append(chunk_size, rand[0]);
      sink.Append(chunk_size, rand[0]);
    }
    sink.Flush();
    EXPECT_EQ(actual, expected);
  }
}
}  // namespace
