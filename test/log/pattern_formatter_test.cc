#include <test/testing/log_includes.h>

// log to str and return it
template<typename... Args>

static std::string log_to_str(const std::string &msg, const Args &... args) {
    std::ostringstream oss;
    auto oss_sink = std::make_shared<abel::log::sinks::ostream_sink_mt>(oss);
    abel::log::logger oss_logger("pattern_tester", oss_sink);
    oss_logger.set_level(abel::log::info);

    oss_logger.set_formatter(std::unique_ptr<abel::log::formatter>(new abel::log::pattern_formatter(args...)));

    oss_logger.info(msg);
    return oss.str();
}

TEST(patternformatter, empty) {
    std::string msg = "Hello custom eol test";
    std::string eol = ";)";
    // auto formatter = std::make_shared<abel::pattern_formatter>("%v", abel::pattern_time_type::local, ";)");
    std::unique_ptr<abel::log::formatter>(
            new abel::log::pattern_formatter("%v", abel::log::pattern_time_type::local, ";)"));

    EXPECT_TRUE(log_to_str(msg, "%v", abel::log::pattern_time_type::local, ";)") == msg + eol);
}

TEST(patternformatter, empty1) {
    EXPECT_TRUE(log_to_str("Some message", "", abel::log::pattern_time_type::local, "") == "");
}

TEST(patternformatter, empty2) {
    EXPECT_TRUE(log_to_str("Some message", "", abel::log::pattern_time_type::local, "\n") == "\n");
}

TEST(patternformatter, level) {
    EXPECT_TRUE(
            log_to_str("Some message", "[%l] %v", abel::log::pattern_time_type::local, "\n") ==
            "[info] Some message\n");
}

TEST(patternformatter, shortname) {
    EXPECT_TRUE(
            log_to_str("Some message", "[%L] %v", abel::log::pattern_time_type::local, "\n") == "[I] Some message\n");
}

TEST(patternformatter, name) {
    EXPECT_TRUE(log_to_str("Some message", "[%n] %v", abel::log::pattern_time_type::local, "\n")
                == "[pattern_tester] Some message\n");
}

TEST(patternformatter, date) {
    auto now_tm = abel::local_tm(abel::now());
    std::stringstream oss;
    oss << std::setfill('0') << std::setw(2) << now_tm.tm_mon + 1 << "/" << std::setw(2) << now_tm.tm_mday << "/"
        << std::setw(2)
        << (now_tm.tm_year + 1900) % 1000 << " Some message\n";
    EXPECT_TRUE(log_to_str("Some message", "%D %v", abel::log::pattern_time_type::local, "\n") == oss.str());
}

TEST(patternformatter, color) {
    auto formatter = std::make_shared<abel::log::pattern_formatter>("%^%v%$", abel::log::pattern_time_type::local,
                                                                    "\n");
    abel::log::details::log_msg msg;
    fmt::format_to(msg.raw, "Hello");
    fmt::memory_buffer formatted;
    formatter->format(msg, formatted);
    EXPECT_TRUE(msg.color_range_start == 0);
    EXPECT_TRUE(msg.color_range_end == 5);
    EXPECT_TRUE(log_to_str("hello", "%^%v%$", abel::log::pattern_time_type::local, "\n") == "hello\n");
}

TEST(patternformatter, color2) {
    auto formatter = std::make_shared<abel::log::pattern_formatter>("%^%$", abel::log::pattern_time_type::local, "\n");
    abel::log::details::log_msg msg;
    fmt::memory_buffer formatted;
    formatter->format(msg, formatted);
    EXPECT_TRUE(msg.color_range_start == 0);
    EXPECT_TRUE(msg.color_range_end == 0);
    EXPECT_TRUE(log_to_str("", "%^%$", abel::log::pattern_time_type::local, "\n") == "\n");
}

TEST(patternformatter, color3) {
    auto formatter = std::make_shared<abel::log::pattern_formatter>("%^***%$");
    abel::log::details::log_msg msg;
    fmt::memory_buffer formatted;
    formatter->format(msg, formatted);
    EXPECT_TRUE(msg.color_range_start == 0);
    EXPECT_TRUE(msg.color_range_end == 3);
}

TEST(patternformatter, color4) {
    auto formatter = std::make_shared<abel::log::pattern_formatter>("XX%^YYY%$", abel::log::pattern_time_type::local,
                                                                    "\n");
    abel::log::details::log_msg msg;
    fmt::format_to(msg.raw, "ignored");
    fmt::memory_buffer formatted;
    formatter->format(msg, formatted);
    EXPECT_TRUE(msg.color_range_start == 2);
    EXPECT_TRUE(msg.color_range_end == 5);
    EXPECT_TRUE(log_to_str("ignored", "XX%^YYY%$", abel::log::pattern_time_type::local, "\n") == "XXYYY\n");
}

TEST(patternformatter, color5) {
    auto formatter = std::make_shared<abel::log::pattern_formatter>("**%^");
    abel::log::details::log_msg msg;
    fmt::memory_buffer formatted;
    formatter->format(msg, formatted);
    EXPECT_TRUE(msg.color_range_start == 2);
    EXPECT_TRUE(msg.color_range_end == 0);
}

TEST(patternformatter, color6) {
    auto formatter = std::make_shared<abel::log::pattern_formatter>("**%$");
    abel::log::details::log_msg msg;
    fmt::memory_buffer formatted;
    formatter->format(msg, formatted);
    EXPECT_TRUE(msg.color_range_start == 0);
    EXPECT_TRUE(msg.color_range_end == 2);
}
