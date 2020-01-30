#include <test/testing/log_includes.h>
#include "sink_test.h"

template<class T>
std::string log_info (const T &what, abel::level::level_enum logger_level = abel::level::info) {

    std::ostringstream oss;
    auto oss_sink = std::make_shared<abel::sinks::ostream_sink_mt>(oss);

    abel::logger oss_logger("oss", oss_sink);
    oss_logger.set_level(logger_level);
    oss_logger.set_pattern("%v");
    oss_logger.info(what);

    return oss.str().substr(0, oss.str().length() - strlen(abel::details::default_eol));
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
    EXPECT_TRUE(log_info("Hello", abel::level::err) == "");
    EXPECT_TRUE(log_info("Hello", abel::level::critical) == "");
    EXPECT_TRUE(log_info("Hello", abel::level::info) == "Hello");
    EXPECT_TRUE(log_info("Hello", abel::level::debug) == "Hello");
    EXPECT_TRUE(log_info("Hello", abel::level::trace) == "Hello");
}

TEST(log_levels, convert_to_c_str) {
    EXPECT_TRUE(std::string(abel::level::to_c_str(abel::level::trace)) == "trace");
    EXPECT_TRUE(std::string(abel::level::to_c_str(abel::level::debug)) == "debug");
    EXPECT_TRUE(std::string(abel::level::to_c_str(abel::level::info)) == "info");
    EXPECT_TRUE(std::string(abel::level::to_c_str(abel::level::warn)) == "warning");
    EXPECT_TRUE(std::string(abel::level::to_c_str(abel::level::err)) == "error");
    EXPECT_TRUE(std::string(abel::level::to_c_str(abel::level::critical)) == "critical");
    EXPECT_TRUE(std::string(abel::level::to_c_str(abel::level::off)) == "off");
}

TEST(log_levels, convert_to_short_c_str) {
    EXPECT_TRUE(std::string(abel::level::to_short_c_str(abel::level::trace)) == "T");
    EXPECT_TRUE(std::string(abel::level::to_short_c_str(abel::level::debug)) == "D");
    EXPECT_TRUE(std::string(abel::level::to_short_c_str(abel::level::info)) == "I");
    EXPECT_TRUE(std::string(abel::level::to_short_c_str(abel::level::warn)) == "W");
    EXPECT_TRUE(std::string(abel::level::to_short_c_str(abel::level::err)) == "E");
    EXPECT_TRUE(std::string(abel::level::to_short_c_str(abel::level::critical)) == "C");
    EXPECT_TRUE(std::string(abel::level::to_short_c_str(abel::level::off)) == "O");
}

TEST(log_levels, convert_to_level_enum) {
    EXPECT_TRUE(abel::level::from_str("trace") == abel::level::trace);
    EXPECT_TRUE(abel::level::from_str("debug") == abel::level::debug);
    EXPECT_TRUE(abel::level::from_str("info") == abel::level::info);
    EXPECT_TRUE(abel::level::from_str("warning") == abel::level::warn);
    EXPECT_TRUE(abel::level::from_str("error") == abel::level::err);
    EXPECT_TRUE(abel::level::from_str("critical") == abel::level::critical);
    EXPECT_TRUE(abel::level::from_str("off") == abel::level::off);
    EXPECT_TRUE(abel::level::from_str("null") == abel::level::off);
}

TEST(log_levels, periodic_flush) {
    using namespace abel;

    auto logger = abel::create<sinks::test_sink_mt>("periodic_flush");

    auto test_sink = std::static_pointer_cast<sinks::test_sink_mt>(logger->sinks()[0]);

    abel::flush_every(std::chrono::seconds(1));
    std::this_thread::sleep_for(std::chrono::milliseconds(1100));
    EXPECT_TRUE(test_sink->flush_counter() == 1);
    abel::flush_every(std::chrono::seconds(0));
    abel::drop_all();
}
