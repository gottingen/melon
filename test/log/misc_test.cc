#include <test/testing/log_includes.h>
#include "sink_test.h"

template<class T>
std::string log_info (const T &what, spdlog::level::level_enum logger_level = spdlog::level::info) {

    std::ostringstream oss;
    auto oss_sink = std::make_shared<spdlog::sinks::ostream_sink_mt>(oss);

    spdlog::logger oss_logger("oss", oss_sink);
    oss_logger.set_level(logger_level);
    oss_logger.set_pattern("%v");
    oss_logger.info(what);

    return oss.str().substr(0, oss.str().length() - strlen(spdlog::details::os::default_eol));
}

TEST(basic_logging, basic_logging) {
    // const char
    EXPECT_TRUE(log_info("Hello") == "Hello");
    EXPECT_TRUE(log_info("") == "");

    // std::string
    EXPECT_TRUE(log_info(std::string("Hello")) == "Hello");
    EXPECT_TRUE(log_info(std::string()) == std::string());

    // Numbers
    EXPECT_TRUE(log_info(5) == "5");
    EXPECT_TRUE(log_info(5.6) == "5.6");

    // User defined class
    // EXPECT_TRUE(log_info(some_logged_class("some_val")) == "some_val");
}

TEST(log_levels, log_levels) {
    EXPECT_TRUE(log_info("Hello", spdlog::level::err) == "");
    EXPECT_TRUE(log_info("Hello", spdlog::level::critical) == "");
    EXPECT_TRUE(log_info("Hello", spdlog::level::info) == "Hello");
    EXPECT_TRUE(log_info("Hello", spdlog::level::debug) == "Hello");
    EXPECT_TRUE(log_info("Hello", spdlog::level::trace) == "Hello");
}

TEST(log_levels, convert_to_c_str) {
    EXPECT_TRUE(std::string(spdlog::level::to_c_str(spdlog::level::trace)) == "trace");
    EXPECT_TRUE(std::string(spdlog::level::to_c_str(spdlog::level::debug)) == "debug");
    EXPECT_TRUE(std::string(spdlog::level::to_c_str(spdlog::level::info)) == "info");
    EXPECT_TRUE(std::string(spdlog::level::to_c_str(spdlog::level::warn)) == "warning");
    EXPECT_TRUE(std::string(spdlog::level::to_c_str(spdlog::level::err)) == "error");
    EXPECT_TRUE(std::string(spdlog::level::to_c_str(spdlog::level::critical)) == "critical");
    EXPECT_TRUE(std::string(spdlog::level::to_c_str(spdlog::level::off)) == "off");
}

TEST(log_levels, convert_to_short_c_str) {
    EXPECT_TRUE(std::string(spdlog::level::to_short_c_str(spdlog::level::trace)) == "T");
    EXPECT_TRUE(std::string(spdlog::level::to_short_c_str(spdlog::level::debug)) == "D");
    EXPECT_TRUE(std::string(spdlog::level::to_short_c_str(spdlog::level::info)) == "I");
    EXPECT_TRUE(std::string(spdlog::level::to_short_c_str(spdlog::level::warn)) == "W");
    EXPECT_TRUE(std::string(spdlog::level::to_short_c_str(spdlog::level::err)) == "E");
    EXPECT_TRUE(std::string(spdlog::level::to_short_c_str(spdlog::level::critical)) == "C");
    EXPECT_TRUE(std::string(spdlog::level::to_short_c_str(spdlog::level::off)) == "O");
}

TEST(log_levels, convert_to_level_enum) {
    EXPECT_TRUE(spdlog::level::from_str("trace") == spdlog::level::trace);
    EXPECT_TRUE(spdlog::level::from_str("debug") == spdlog::level::debug);
    EXPECT_TRUE(spdlog::level::from_str("info") == spdlog::level::info);
    EXPECT_TRUE(spdlog::level::from_str("warning") == spdlog::level::warn);
    EXPECT_TRUE(spdlog::level::from_str("error") == spdlog::level::err);
    EXPECT_TRUE(spdlog::level::from_str("critical") == spdlog::level::critical);
    EXPECT_TRUE(spdlog::level::from_str("off") == spdlog::level::off);
    EXPECT_TRUE(spdlog::level::from_str("null") == spdlog::level::off);
}

TEST(log_levels, periodic_flush) {
    using namespace spdlog;

    auto logger = spdlog::create<sinks::test_sink_mt>("periodic_flush");

    auto test_sink = std::static_pointer_cast<sinks::test_sink_mt>(logger->sinks()[0]);

    spdlog::flush_every(std::chrono::seconds(1));
    std::this_thread::sleep_for(std::chrono::milliseconds(1100));
    EXPECT_TRUE(test_sink->flush_counter() == 1);
    spdlog::flush_every(std::chrono::seconds(0));
    spdlog::drop_all();
}
