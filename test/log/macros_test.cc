
#include <test/testing/log_includes.h>

TEST(macros, debug) {
    prepare_logdir();
    std::string filename = "logs/simple_log";

    auto logger = spdlog::create<spdlog::sinks::basic_file_sink_mt>("logger", filename);
    logger->set_pattern("%v");
    logger->set_level(spdlog::level::trace);

    SPDLOG_TRACE(logger, "Test message 1");
    SPDLOG_DEBUG(logger, "Test message 2");
    logger->flush();

    EXPECT_TRUE(ends_with(file_contents(filename), "Test message 2\n"));
    EXPECT_TRUE(count_lines(filename) == 2);
}

TEST(macros, debug_string) {
    prepare_logdir();
    std::string filename = "logs/simple_log";

    auto logger = spdlog::create<spdlog::sinks::basic_file_sink_mt>("logger", filename);
    logger->set_pattern("%v");
    logger->set_level(spdlog::level::trace);

    SPDLOG_TRACE(logger, "Test message {}", 1);
    // SPDLOG_DEBUG(logger, "Test message 2");
    SPDLOG_DEBUG(logger, "Test message {}", 222);
    logger->flush();

    EXPECT_TRUE(ends_with(file_contents(filename), "Test message 222\n"));
    EXPECT_TRUE(count_lines(filename) == 2);
}
