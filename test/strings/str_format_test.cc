
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <string>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <abel/strings/str_format.h>
#include <abel/strings/string_view.h>

namespace abel {

namespace {
using format_internal::FormatArgImpl;

using FormatEntryPointTest = ::testing::Test;

TEST_F(FormatEntryPointTest, format) {
  std::string sink;
  EXPECT_TRUE(format(&sink, "A format %d", 123));
  EXPECT_EQ("A format 123", sink);
  sink.clear();

  parsed_format<'d'> pc("A format %d");
  EXPECT_TRUE(format(&sink, pc, 123));
  EXPECT_EQ("A format 123", sink);
}
TEST_F(FormatEntryPointTest, UntypedFormat) {
  constexpr const char* formats[] = {
    "",
    "a",
    "%80d",
#if !defined(_MSC_VER) && !defined(__ANDROID__) && !defined(__native_client__)
    // MSVC, NaCL and Android don't support positional syntax.
    "complicated multipart %% %1$d format %1$0999d",
#endif  // _MSC_VER
  };
  for (const char* fmt : formats) {
    std::string actual;
    int i = 123;
    FormatArgImpl arg_123(i);
    abel::Span<const FormatArgImpl> args(&arg_123, 1);
    untyped_format_spec format(fmt);

    EXPECT_TRUE(FormatUntyped(&actual, format, args));
    char buf[4096]{};
    snprintf(buf, sizeof(buf), fmt, 123);
    EXPECT_EQ(
        format_internal::FormatPack(
            format_internal::UntypedFormatSpecImpl::Extract(format), args),
        buf);
    EXPECT_EQ(actual, buf);
  }
  // The internal version works with a preparsed format.
  parsed_format<'d'> pc("A format %d");
  int i = 345;
  format_arg arg(i);
  std::string out;
  EXPECT_TRUE(format_internal::FormatUntyped(
      &out, format_internal::UntypedFormatSpecImpl(&pc), {&arg, 1}));
  EXPECT_EQ("A format 345", out);
}

TEST_F(FormatEntryPointTest, StringFormat) {
  EXPECT_EQ("123", string_format("%d", 123));
  constexpr abel::string_view view("=%d=", 4);
  EXPECT_EQ("=123=", string_format(view, 123));
}

TEST_F(FormatEntryPointTest, AppendFormat) {
  std::string s;
  std::string& r = string_append_format(&s, "%d", 123);
  EXPECT_EQ(&s, &r);  // should be same object
  EXPECT_EQ("123", r);
}

TEST_F(FormatEntryPointTest, AppendFormatFail) {
  std::string s = "orig";

  untyped_format_spec format(" more %d");
  FormatArgImpl arg("not an int");

  EXPECT_EQ("orig",
            format_internal::AppendPack(
                &s, format_internal::UntypedFormatSpecImpl::Extract(format),
                {&arg, 1}));
}


TEST_F(FormatEntryPointTest, ManyArgs) {
  EXPECT_EQ("24", string_format("%24$d", 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13,
                            14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24));
  EXPECT_EQ("60", string_format("%60$d", 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13,
                            14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26,
                            27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39,
                            40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52,
                            53, 54, 55, 56, 57, 58, 59, 60));
}

TEST_F(FormatEntryPointTest, Preparsed) {
  parsed_format<'d'> pc("%d");
  EXPECT_EQ("123", string_format(pc, 123));
  // rvalue ok?
  EXPECT_EQ("123", string_format(parsed_format<'d'>("%d"), 123));
  constexpr abel::string_view view("=%d=", 4);
  EXPECT_EQ("=123=", string_format(parsed_format<'d'>(view), 123));
}

TEST_F(FormatEntryPointTest, format_count_capture) {
  int n = 0;
  EXPECT_EQ("", string_format("%n", format_count_capture(&n)));
  EXPECT_EQ(0, n);
  EXPECT_EQ("123", string_format("%d%n", 123, format_count_capture(&n)));
  EXPECT_EQ(3, n);
}

TEST_F(FormatEntryPointTest, FormatCountCaptureWrongType) {
  // Should reject int*.
  int n = 0;
  untyped_format_spec format("%d%n");
  int i = 123, *ip = &n;
  FormatArgImpl args[2] = {FormatArgImpl(i), FormatArgImpl(ip)};

  EXPECT_EQ("", format_internal::FormatPack(
                    format_internal::UntypedFormatSpecImpl::Extract(format),
                    abel::MakeSpan(args)));
}

TEST_F(FormatEntryPointTest, FormatCountCaptureMultiple) {
  int n1 = 0;
  int n2 = 0;
  EXPECT_EQ("    1         2",
            string_format("%5d%n%10d%n", 1, format_count_capture(&n1), 2,
                      format_count_capture(&n2)));
  EXPECT_EQ(5, n1);
  EXPECT_EQ(15, n2);
}

TEST_F(FormatEntryPointTest, FormatCountCaptureExample) {
  int n;
  std::string s;
  string_append_format(&s, "%s: %n%s\n", "(1,1)", format_count_capture(&n), "(1,2)");
  string_append_format(&s, "%*s%s\n", n, "", "(2,2)");
  EXPECT_EQ(7, n);
  EXPECT_EQ(
      "(1,1): (1,2)\n"
      "       (2,2)\n",
      s);
}

TEST_F(FormatEntryPointTest, Stream) {
  const std::string formats[] = {
    "",
    "a",
    "%80d",
    "%d %u %c %s %f %g",
#if !defined(_MSC_VER) && !defined(__ANDROID__) && !defined(__native_client__)
    // MSVC, NaCL and Android don't support positional syntax.
    "complicated multipart %% %1$d format %1$080d",
#endif  // _MSC_VER
  };
  std::string buf(4096, '\0');
  for (const auto& fmt : formats) {
    const auto parsed =
        parsed_format<'d', 'u', 'c', 's', 'f', 'g'>::NewAllowIgnored(fmt);
    std::ostringstream oss;
    oss << stream_format(*parsed, 123, 3, 49, "multistreaming!!!", 1.01, 1.01);
    int fmt_result = snprintf(&*buf.begin(), buf.size(), fmt.c_str(),  //
                                 123, 3, 49, "multistreaming!!!", 1.01, 1.01);
    ASSERT_TRUE(oss) << fmt;
    ASSERT_TRUE(fmt_result >= 0 && static_cast<size_t>(fmt_result) < buf.size())
        << fmt_result;
    EXPECT_EQ(buf.c_str(), oss.str());
  }
}

TEST_F(FormatEntryPointTest, StreamOk) {
  std::ostringstream oss;
  oss << stream_format("hello %d", 123);
  EXPECT_EQ("hello 123", oss.str());
  EXPECT_TRUE(oss.good());
}

TEST_F(FormatEntryPointTest, StreamFail) {
  std::ostringstream oss;
  untyped_format_spec format("hello %d");
  FormatArgImpl arg("non-numeric");
  oss << format_internal::Streamable(
      format_internal::UntypedFormatSpecImpl::Extract(format), {&arg, 1});
  EXPECT_EQ("hello ", oss.str());  // partial write
  EXPECT_TRUE(oss.fail());
}

std::string WithSnprintf(const char* fmt, ...) {
  std::string buf;
  buf.resize(128);
  va_list va;
  va_start(va, fmt);
  int r = vsnprintf(&*buf.begin(), buf.size(), fmt, va);
  va_end(va);
  EXPECT_GE(r, 0);
  EXPECT_LT(r, buf.size());
  buf.resize(r);
  return buf;
}

TEST_F(FormatEntryPointTest, FloatPrecisionArg) {
  // Test that positional parameters for width and precision
  // are indexed to precede the value.
  // Also sanity check the same formats against snprintf.
  EXPECT_EQ("0.1", string_format("%.1f", 0.1));
  EXPECT_EQ("0.1", WithSnprintf("%.1f", 0.1));
  EXPECT_EQ("  0.1", string_format("%*.1f", 5, 0.1));
  EXPECT_EQ("  0.1", WithSnprintf("%*.1f", 5, 0.1));
  EXPECT_EQ("0.1", string_format("%.*f", 1, 0.1));
  EXPECT_EQ("0.1", WithSnprintf("%.*f", 1, 0.1));
  EXPECT_EQ("  0.1", string_format("%*.*f", 5, 1, 0.1));
  EXPECT_EQ("  0.1", WithSnprintf("%*.*f", 5, 1, 0.1));
}
namespace streamed_test {
struct X {};
std::ostream& operator<<(std::ostream& os, const X&) {
  return os << "X";
}
}  // streamed_test

TEST_F(FormatEntryPointTest, FormatStreamed) {
  EXPECT_EQ("123", string_format("%s", FormatStreamed(123)));
  EXPECT_EQ("  123", string_format("%5s", FormatStreamed(123)));
  EXPECT_EQ("123  ", string_format("%-5s", FormatStreamed(123)));
  EXPECT_EQ("X", string_format("%s", FormatStreamed(streamed_test::X())));
  EXPECT_EQ("123", string_format("%s", FormatStreamed(stream_format("%d", 123))));
}

// Helper class that creates a temporary file and exposes a FILE* to it.
// It will close the file on destruction.
class TempFile {
 public:
  TempFile() : file_(std::tmpfile()) {}
  ~TempFile() { std::fclose(file_); }

  std::FILE* file() const { return file_; }

  // Read the file into a std::string.
  std::string ReadFile() {
    std::fseek(file_, 0, SEEK_END);
    int size = std::ftell(file_);
    EXPECT_GT(size, 0);
    std::rewind(file_);
    std::string str(2 * size, ' ');
    int read_bytes = std::fread(&str[0], 1, str.size(), file_);
    EXPECT_EQ(read_bytes, size);
    str.resize(read_bytes);
    EXPECT_TRUE(std::feof(file_));
    return str;
  }

 private:
  std::FILE* file_;
};

TEST_F(FormatEntryPointTest, string_fprintf) {
  TempFile tmp;
  int result =
      string_fprintf(tmp.file(), "STRING: %s NUMBER: %010d", std::string("ABC"), -19);
  EXPECT_EQ(result, 30);
  EXPECT_EQ(tmp.ReadFile(), "STRING: ABC NUMBER: -000000019");
}

TEST_F(FormatEntryPointTest, FPrintFError) {
  errno = 0;
  int result = string_fprintf(stdin, "ABC");
  EXPECT_LT(result, 0);
  EXPECT_EQ(errno, EBADF);
}

#ifdef __GLIBC__
TEST_F(FormatEntryPointTest, FprintfTooLarge) {
  std::FILE* f = std::fopen("/dev/null", "w");
  int width = 2000000000;
  errno = 0;
  int result = string_fprintf(f, "%*d %*d", width, 0, width, 0);
  EXPECT_LT(result, 0);
  EXPECT_EQ(errno, EFBIG);
  std::fclose(f);
}

TEST_F(FormatEntryPointTest, string_printf) {
  int stdout_tmp = dup(STDOUT_FILENO);

  TempFile tmp;
  std::fflush(stdout);
  dup2(fileno(tmp.file()), STDOUT_FILENO);

  int result = string_printf("STRING: %s NUMBER: %010d", std::string("ABC"), -19);

  std::fflush(stdout);
  dup2(stdout_tmp, STDOUT_FILENO);
  close(stdout_tmp);

  EXPECT_EQ(result, 30);
  EXPECT_EQ(tmp.ReadFile(), "STRING: ABC NUMBER: -000000019");
}
#endif  // __GLIBC__

TEST_F(FormatEntryPointTest, string_snprintf) {
  char buffer[16];
  int result =
      string_snprintf(buffer, sizeof(buffer), "STRING: %s", std::string("ABC"));
  EXPECT_EQ(result, 11);
  EXPECT_EQ(std::string(buffer), "STRING: ABC");

  result = string_snprintf(buffer, sizeof(buffer), "NUMBER: %d", 123456);
  EXPECT_EQ(result, 14);
  EXPECT_EQ(std::string(buffer), "NUMBER: 123456");

  result = string_snprintf(buffer, sizeof(buffer), "NUMBER: %d", 1234567);
  EXPECT_EQ(result, 15);
  EXPECT_EQ(std::string(buffer), "NUMBER: 1234567");

  result = string_snprintf(buffer, sizeof(buffer), "NUMBER: %d", 12345678);
  EXPECT_EQ(result, 16);
  EXPECT_EQ(std::string(buffer), "NUMBER: 1234567");

  result = string_snprintf(buffer, sizeof(buffer), "NUMBER: %d", 123456789);
  EXPECT_EQ(result, 17);
  EXPECT_EQ(std::string(buffer), "NUMBER: 1234567");

  result = string_snprintf(nullptr, 0, "Just checking the %s of the output.", "size");
  EXPECT_EQ(result, 37);
}

TEST(string_format, BehavesAsDocumented) {
  std::string s = abel::string_format("%s, %d!", "Hello", 123);
  EXPECT_EQ("Hello, 123!", s);
  // The format of a replacement is
  // '%'[position][flags][width['.'precision]][length_modifier][format]
  EXPECT_EQ(abel::string_format("%1$+3.2Lf", 1.1), "+1.10");
  // Text conversion:
  //     "c" - Character.              Eg: 'a' -> "A", 20 -> " "
  EXPECT_EQ(string_format("%c", 'a'), "a");
  EXPECT_EQ(string_format("%c", 0x20), " ");
  //           Formats char and integral types: int, long, uint64_t, etc.
  EXPECT_EQ(string_format("%c", int{'a'}), "a");
  EXPECT_EQ(string_format("%c", long{'a'}), "a");  // NOLINT
  EXPECT_EQ(string_format("%c", uint64_t{'a'}), "a");
  //     "s" - std::string       Eg: "C" -> "C", std::string("C++") -> "C++"
  //           Formats std::string, char*, string_view, and Cord.
  EXPECT_EQ(string_format("%s", "C"), "C");
  EXPECT_EQ(string_format("%s", std::string("C++")), "C++");
  EXPECT_EQ(string_format("%s", string_view("view")), "view");
  // Integral Conversion
  //     These format integral types: char, int, long, uint64_t, etc.
  EXPECT_EQ(string_format("%d", char{10}), "10");
  EXPECT_EQ(string_format("%d", int{10}), "10");
  EXPECT_EQ(string_format("%d", long{10}), "10");  // NOLINT
  EXPECT_EQ(string_format("%d", uint64_t{10}), "10");
  //     d,i - signed decimal          Eg: -10 -> "-10"
  EXPECT_EQ(string_format("%d", -10), "-10");
  EXPECT_EQ(string_format("%i", -10), "-10");
  //      o  - octal                   Eg:  10 -> "12"
  EXPECT_EQ(string_format("%o", 10), "12");
  //      u  - unsigned decimal        Eg:  10 -> "10"
  EXPECT_EQ(string_format("%u", 10), "10");
  //     x/X - lower,upper case hex    Eg:  10 -> "a"/"A"
  EXPECT_EQ(string_format("%x", 10), "a");
  EXPECT_EQ(string_format("%X", 10), "A");
  // Floating-point, with upper/lower-case output.
  //     These format floating points types: float, double, long double, etc.
  EXPECT_EQ(string_format("%.1f", float{1}), "1.0");
  EXPECT_EQ(string_format("%.1f", double{1}), "1.0");
  const long double long_double = 1.0;
  EXPECT_EQ(string_format("%.1f", long_double), "1.0");
  //     These also format integral types: char, int, long, uint64_t, etc.:
  EXPECT_EQ(string_format("%.1f", char{1}), "1.0");
  EXPECT_EQ(string_format("%.1f", int{1}), "1.0");
  EXPECT_EQ(string_format("%.1f", long{1}), "1.0");  // NOLINT
  EXPECT_EQ(string_format("%.1f", uint64_t{1}), "1.0");
  //     f/F - decimal.                Eg: 123456789 -> "123456789.000000"
  EXPECT_EQ(string_format("%f", 123456789), "123456789.000000");
  EXPECT_EQ(string_format("%F", 123456789), "123456789.000000");
  //     e/E - exponentiated           Eg: .01 -> "1.00000e-2"/"1.00000E-2"
  EXPECT_EQ(string_format("%e", .01), "1.000000e-02");
  EXPECT_EQ(string_format("%E", .01), "1.000000E-02");
  //     g/G - exponentiate to fit     Eg: .01 -> "0.01", 1e10 ->"1e+10"/"1E+10"
  EXPECT_EQ(string_format("%g", .01), "0.01");
  EXPECT_EQ(string_format("%g", 1e10), "1e+10");
  EXPECT_EQ(string_format("%G", 1e10), "1E+10");
  //     a/A - lower,upper case hex    Eg: -3.0 -> "-0x1.8p+1"/"-0X1.8P+1"

// On Android platform <=21, there is a regression in hexfloat formatting.
#if !defined(__ANDROID_API__) || __ANDROID_API__ > 21
  EXPECT_EQ(string_format("%.1a", -3.0), "-0x1.8p+1");  // .1 to fix MSVC output
  EXPECT_EQ(string_format("%.1A", -3.0), "-0X1.8P+1");  // .1 to fix MSVC output
#endif

  // Other conversion
  int64_t value = 0x7ffdeb4;
  auto ptr_value = static_cast<uintptr_t>(value);
  const int& something = *reinterpret_cast<const int*>(ptr_value);
  EXPECT_EQ(string_format("%p", &something), string_format("0x%x", ptr_value));

  // Output widths are supported, with optional flags.
  EXPECT_EQ(string_format("%3d", 1), "  1");
  EXPECT_EQ(string_format("%3d", 123456), "123456");
  EXPECT_EQ(string_format("%06.2f", 1.234), "001.23");
  EXPECT_EQ(string_format("%+d", 1), "+1");
  EXPECT_EQ(string_format("% d", 1), " 1");
  EXPECT_EQ(string_format("%-4d", -1), "-1  ");
  EXPECT_EQ(string_format("%#o", 10), "012");
  EXPECT_EQ(string_format("%#x", 15), "0xf");
  EXPECT_EQ(string_format("%04d", 8), "0008");
  // Posix positional substitution.
  EXPECT_EQ(abel::string_format("%2$s, %3$s, %1$s!", "vici", "veni", "vidi"),
            "veni, vidi, vici!");
  // Length modifiers are ignored.
  EXPECT_EQ(string_format("%hhd", int{1}), "1");
  EXPECT_EQ(string_format("%hd", int{1}), "1");
  EXPECT_EQ(string_format("%ld", int{1}), "1");
  EXPECT_EQ(string_format("%lld", int{1}), "1");
  EXPECT_EQ(string_format("%Ld", int{1}), "1");
  EXPECT_EQ(string_format("%jd", int{1}), "1");
  EXPECT_EQ(string_format("%zd", int{1}), "1");
  EXPECT_EQ(string_format("%td", int{1}), "1");
  EXPECT_EQ(string_format("%qd", int{1}), "1");
}

using format_internal::ExtendedParsedFormat;
using format_internal::ParsedFormatBase;

struct SummarizeConsumer {
  std::string* out;
  explicit SummarizeConsumer(std::string* out) : out(out) {}

  bool Append(string_view s) {
    *out += "[" + std::string(s) + "]";
    return true;
  }

  bool ConvertOne(const format_internal::UnboundConversion& conv,
                  string_view s) {
    *out += "{";
    *out += std::string(s);
    *out += ":";
    *out += std::to_string(conv.arg_position) + "$";
    if (conv.width.is_from_arg()) {
      *out += std::to_string(conv.width.get_from_arg()) + "$*";
    }
    if (conv.precision.is_from_arg()) {
      *out += "." + std::to_string(conv.precision.get_from_arg()) + "$*";
    }
    *out += conv.conv.Char();
    *out += "}";
    return true;
  }
};

std::string SummarizeParsedFormat(const ParsedFormatBase& pc) {
  std::string out;
  if (!pc.ProcessFormat(SummarizeConsumer(&out))) out += "!";
  return out;
}

using ParsedFormatTest = ::testing::Test;

TEST_F(ParsedFormatTest, SimpleChecked) {
  EXPECT_EQ("[ABC]{d:1$d}[DEF]",
            SummarizeParsedFormat(parsed_format<'d'>("ABC%dDEF")));
  EXPECT_EQ("{s:1$s}[FFF]{d:2$d}[ZZZ]{f:3$f}",
            SummarizeParsedFormat(parsed_format<'s', 'd', 'f'>("%sFFF%dZZZ%f")));
  EXPECT_EQ("{s:1$s}[ ]{.*d:3$.2$*d}",
            SummarizeParsedFormat(parsed_format<'s', '*', 'd'>("%s %.*d")));
}

TEST_F(ParsedFormatTest, SimpleUncheckedCorrect) {
  auto f = parsed_format<'d'>::New("ABC%dDEF");
  ASSERT_TRUE(f);
  EXPECT_EQ("[ABC]{d:1$d}[DEF]", SummarizeParsedFormat(*f));

  std::string format = "%sFFF%dZZZ%f";
  auto f2 = parsed_format<'s', 'd', 'f'>::New(format);

  ASSERT_TRUE(f2);
  EXPECT_EQ("{s:1$s}[FFF]{d:2$d}[ZZZ]{f:3$f}", SummarizeParsedFormat(*f2));

  f2 = parsed_format<'s', 'd', 'f'>::New("%s %d %f");

  ASSERT_TRUE(f2);
  EXPECT_EQ("{s:1$s}[ ]{d:2$d}[ ]{f:3$f}", SummarizeParsedFormat(*f2));

  auto star = parsed_format<'*', 'd'>::New("%*d");
  ASSERT_TRUE(star);
  EXPECT_EQ("{*d:2$1$*d}", SummarizeParsedFormat(*star));

  auto dollar = parsed_format<'d', 's'>::New("%2$s %1$d");
  ASSERT_TRUE(dollar);
  EXPECT_EQ("{2$s:2$s}[ ]{1$d:1$d}", SummarizeParsedFormat(*dollar));
  // with reuse
  dollar = parsed_format<'d', 's'>::New("%2$s %1$d %1$d");
  ASSERT_TRUE(dollar);
  EXPECT_EQ("{2$s:2$s}[ ]{1$d:1$d}[ ]{1$d:1$d}",
            SummarizeParsedFormat(*dollar));
}

TEST_F(ParsedFormatTest, SimpleUncheckedIgnoredArgs) {
  EXPECT_FALSE((parsed_format<'d', 's'>::New("ABC")));
  EXPECT_FALSE((parsed_format<'d', 's'>::New("%dABC")));
  EXPECT_FALSE((parsed_format<'d', 's'>::New("ABC%2$s")));
  auto f = parsed_format<'d', 's'>::NewAllowIgnored("ABC");
  ASSERT_TRUE(f);
  EXPECT_EQ("[ABC]", SummarizeParsedFormat(*f));
  f = parsed_format<'d', 's'>::NewAllowIgnored("%dABC");
  ASSERT_TRUE(f);
  EXPECT_EQ("{d:1$d}[ABC]", SummarizeParsedFormat(*f));
  f = parsed_format<'d', 's'>::NewAllowIgnored("ABC%2$s");
  ASSERT_TRUE(f);
  EXPECT_EQ("[ABC]{2$s:2$s}", SummarizeParsedFormat(*f));
}

TEST_F(ParsedFormatTest, SimpleUncheckedUnsupported) {
  EXPECT_FALSE(parsed_format<'d'>::New("%1$d %1$x"));
  EXPECT_FALSE(parsed_format<'x'>::New("%1$d %1$x"));
}

TEST_F(ParsedFormatTest, SimpleUncheckedIncorrect) {
  EXPECT_FALSE(parsed_format<'d'>::New(""));

  EXPECT_FALSE(parsed_format<'d'>::New("ABC%dDEF%d"));

  std::string format = "%sFFF%dZZZ%f";
  EXPECT_FALSE((parsed_format<'s', 'd', 'g'>::New(format)));
}

using format_internal::Conv;

TEST_F(ParsedFormatTest, UncheckedCorrect) {
  auto f = ExtendedParsedFormat<Conv::d>::New("ABC%dDEF");
  ASSERT_TRUE(f);
  EXPECT_EQ("[ABC]{d:1$d}[DEF]", SummarizeParsedFormat(*f));

  std::string format = "%sFFF%dZZZ%f";
  auto f2 =
      ExtendedParsedFormat<Conv::string, Conv::d, Conv::floating>::New(format);

  ASSERT_TRUE(f2);
  EXPECT_EQ("{s:1$s}[FFF]{d:2$d}[ZZZ]{f:3$f}", SummarizeParsedFormat(*f2));

  f2 = ExtendedParsedFormat<Conv::string, Conv::d, Conv::floating>::New(
      "%s %d %f");

  ASSERT_TRUE(f2);
  EXPECT_EQ("{s:1$s}[ ]{d:2$d}[ ]{f:3$f}", SummarizeParsedFormat(*f2));

  auto star = ExtendedParsedFormat<Conv::star, Conv::d>::New("%*d");
  ASSERT_TRUE(star);
  EXPECT_EQ("{*d:2$1$*d}", SummarizeParsedFormat(*star));

  auto dollar = ExtendedParsedFormat<Conv::d, Conv::s>::New("%2$s %1$d");
  ASSERT_TRUE(dollar);
  EXPECT_EQ("{2$s:2$s}[ ]{1$d:1$d}", SummarizeParsedFormat(*dollar));
  // with reuse
  dollar = ExtendedParsedFormat<Conv::d, Conv::s>::New("%2$s %1$d %1$d");
  ASSERT_TRUE(dollar);
  EXPECT_EQ("{2$s:2$s}[ ]{1$d:1$d}[ ]{1$d:1$d}",
            SummarizeParsedFormat(*dollar));
}

TEST_F(ParsedFormatTest, UncheckedIgnoredArgs) {
  EXPECT_FALSE((ExtendedParsedFormat<Conv::d, Conv::s>::New("ABC")));
  EXPECT_FALSE((ExtendedParsedFormat<Conv::d, Conv::s>::New("%dABC")));
  EXPECT_FALSE((ExtendedParsedFormat<Conv::d, Conv::s>::New("ABC%2$s")));
  auto f = ExtendedParsedFormat<Conv::d, Conv::s>::NewAllowIgnored("ABC");
  ASSERT_TRUE(f);
  EXPECT_EQ("[ABC]", SummarizeParsedFormat(*f));
  f = ExtendedParsedFormat<Conv::d, Conv::s>::NewAllowIgnored("%dABC");
  ASSERT_TRUE(f);
  EXPECT_EQ("{d:1$d}[ABC]", SummarizeParsedFormat(*f));
  f = ExtendedParsedFormat<Conv::d, Conv::s>::NewAllowIgnored("ABC%2$s");
  ASSERT_TRUE(f);
  EXPECT_EQ("[ABC]{2$s:2$s}", SummarizeParsedFormat(*f));
}

TEST_F(ParsedFormatTest, UncheckedMultipleTypes) {
  auto dx = ExtendedParsedFormat<Conv::d | Conv::x>::New("%1$d %1$x");
  EXPECT_TRUE(dx);
  EXPECT_EQ("{1$d:1$d}[ ]{1$x:1$x}", SummarizeParsedFormat(*dx));

  dx = ExtendedParsedFormat<Conv::d | Conv::x>::New("%1$d");
  EXPECT_TRUE(dx);
  EXPECT_EQ("{1$d:1$d}", SummarizeParsedFormat(*dx));
}

TEST_F(ParsedFormatTest, UncheckedIncorrect) {
  EXPECT_FALSE(ExtendedParsedFormat<Conv::d>::New(""));

  EXPECT_FALSE(ExtendedParsedFormat<Conv::d>::New("ABC%dDEF%d"));

  std::string format = "%sFFF%dZZZ%f";
  EXPECT_FALSE((ExtendedParsedFormat<Conv::s, Conv::d, Conv::g>::New(format)));
}

TEST_F(ParsedFormatTest, RegressionMixPositional) {
  EXPECT_FALSE((ExtendedParsedFormat<Conv::d, Conv::o>::New("%1$d %o")));
}

using FormatWrapperTest = ::testing::Test;

// Plain wrapper for string_format.
template <typename... Args>
std::string WrappedFormat(const abel::format_spec<Args...>& format,
                          const Args&... args) {
  return string_format(format, args...);
}

TEST_F(FormatWrapperTest, ConstexprStringFormat) {
  EXPECT_EQ(WrappedFormat("%s there", "hello"), "hello there");
}

TEST_F(FormatWrapperTest, parsed_format) {
  parsed_format<'s'> format("%s there");
  EXPECT_EQ(WrappedFormat(format, "hello"), "hello there");
}

}  // namespace

}  // namespace abel

// Some codegen thunks that we can use to easily dump the generated assembly for
// different string_format calls.

std::string CodegenAbelStrFormatInt(int i) {  // NOLINT
  return abel::string_format("%d", i);
}

std::string CodegenAbelStrFormatIntStringInt64(int i, const std::string& s,
                                               int64_t i64) {  // NOLINT
  return abel::string_format("%d %s %d", i, s, i64);
}

void CodegenAbelStrAppendFormatInt(std::string* out, int i) {  // NOLINT
  abel::string_append_format(out, "%d", i);
}

void CodegenAbelStrAppendFormatIntStringInt64(std::string* out, int i,
                                              const std::string& s,
                                              int64_t i64) {  // NOLINT
  abel::string_append_format(out, "%d %s %d", i, s, i64);
}
