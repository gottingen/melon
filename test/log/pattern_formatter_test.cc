#include <test/testing/log_includes.h>

// log to str and return it
template<typename... Args>

static std::string log_to_str (const std::string &msg, const Args &... args) {
    std::ostringstream oss;
    auto oss_sink = std::make_shared<spdlog::sinks::ostream_sink_mt>(oss);
    spdlog::logger oss_logger("pattern_tester", oss_sink);
    oss_logger.set_level(spdlog::level::info);

    oss_logger.set_formatter(std::unique_ptr<spdlog::formatter>(new spdlog::pattern_formatter(args...)));

    oss_logger.info(msg);
    return oss.str();
}

TEST(pattern_formatter, empty) {
    std::string msg = "Hello custom eol test";
    std::string eol = ";)";
    // auto formatter = std::make_shared<spdlog::pattern_formatter>("%v", spdlog::pattern_time_type::local, ";)");
    std::unique_ptr<spdlog::formatter>(new spdlog::pattern_formatter("%v", spdlog::pattern_time_type::local, ";)"));

    EXPECT_TRUE(log_to_str(msg, "%v", spdlog::pattern_time_type::local, ";)") == msg + eol);
}

TEST(pattern_formatter, empty1) {
    EXPECT_TRUE(log_to_str("Some message", "", spdlog::pattern_time_type::local, "") == "");
}

TEST(pattern_formatter, empty2) {
    EXPECT_TRUE(log_to_str("Some message", "", spdlog::pattern_time_type::local, "\n") == "\n");
}

TEST(pattern_formatter, level) {
    EXPECT_TRUE(
        log_to_str("Some message", "[%l] %v", spdlog::pattern_time_type::local, "\n") == "[info] Some message\n");
}

TEST(pattern_formatter, shortname) {
    EXPECT_TRUE(log_to_str("Some message", "[%L] %v", spdlog::pattern_time_type::local, "\n") == "[I] Some message\n");
}

TEST(pattern_formatter, name) {
    EXPECT_TRUE(log_to_str("Some message", "[%n] %v", spdlog::pattern_time_type::local, "\n")
                    == "[pattern_tester] Some message\n");
}

TEST(pattern_formatter, date) {
    auto now_tm = spdlog::details::os::localtime();
    std::stringstream oss;
    oss << std::setfill('0') << std::setw(2) << now_tm.tm_mon + 1 << "/" << std::setw(2) << now_tm.tm_mday << "/"
        << std::setw(2)
        << (now_tm.tm_year + 1900) % 1000 << " Some message\n";
    EXPECT_TRUE(log_to_str("Some message", "%D %v", spdlog::pattern_time_type::local, "\n") == oss.str());
}

TEST(pattern_formatter, color) {
    auto formatter = std::make_shared<spdlog::pattern_formatter>("%^%v%$", spdlog::pattern_time_type::local, "\n");
    spdlog::details::log_msg msg;
    fmt::format_to(msg.raw, "Hello");
    fmt::memory_buffer formatted;
    formatter->format(msg, formatted);
    EXPECT_TRUE(msg.color_range_start == 0);
    EXPECT_TRUE(msg.color_range_end == 5);
    EXPECT_TRUE(log_to_str("hello", "%^%v%$", spdlog::pattern_time_type::local, "\n") == "hello\n");
}

TEST(pattern_formatter, color2) {
    auto formatter = std::make_shared<spdlog::pattern_formatter>("%^%$", spdlog::pattern_time_type::local, "\n");
    spdlog::details::log_msg msg;
    fmt::memory_buffer formatted;
    formatter->format(msg, formatted);
    EXPECT_TRUE(msg.color_range_start == 0);
    EXPECT_TRUE(msg.color_range_end == 0);
    EXPECT_TRUE(log_to_str("", "%^%$", spdlog::pattern_time_type::local, "\n") == "\n");
}

TEST(pattern_formatter, color3) {
    auto formatter = std::make_shared<spdlog::pattern_formatter>("%^***%$");
    spdlog::details::log_msg msg;
    fmt::memory_buffer formatted;
    formatter->format(msg, formatted);
    EXPECT_TRUE(msg.color_range_start == 0);
    EXPECT_TRUE(msg.color_range_end == 3);
}

TEST(pattern_formatter, color4) {
    auto formatter = std::make_shared<spdlog::pattern_formatter>("XX%^YYY%$", spdlog::pattern_time_type::local, "\n");
    spdlog::details::log_msg msg;
    fmt::format_to(msg.raw, "ignored");
    fmt::memory_buffer formatted;
    formatter->format(msg, formatted);
    EXPECT_TRUE(msg.color_range_start == 2);
    EXPECT_TRUE(msg.color_range_end == 5);
    EXPECT_TRUE(log_to_str("ignored", "XX%^YYY%$", spdlog::pattern_time_type::local, "\n") == "XXYYY\n");
}

TEST(pattern_formatter, color5) {
    auto formatter = std::make_shared<spdlog::pattern_formatter>("**%^");
    spdlog::details::log_msg msg;
    fmt::memory_buffer formatted;
    formatter->format(msg, formatted);
    EXPECT_TRUE(msg.color_range_start == 2);
    EXPECT_TRUE(msg.color_range_end == 0);
}

TEST(pattern_formatter, color6) {
    auto formatter = std::make_shared<spdlog::pattern_formatter>("**%$");
    spdlog::details::log_msg msg;
    fmt::memory_buffer formatted;
    formatter->format(msg, formatted);
    EXPECT_TRUE(msg.color_range_start == 0);
    EXPECT_TRUE(msg.color_range_end == 2);
}
