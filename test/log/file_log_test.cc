
#include <test/testing/log_includes.h>

TEST(file_log, simple_file_logger) {
    prepare_logdir();
    std::string filename = "logs/simple_log";

    auto logger = abel_log::create<abel_log::sinks::basic_file_sink_mt>("logger", filename);
    logger->set_pattern("%v");

    logger->info("Test message {}", 1);
    logger->info("Test message {}", 2);

    logger->flush();
    EXPECT_TRUE(file_contents(filename) == std::string("Test message 1\nTest message 2\n"));
    EXPECT_TRUE(count_lines(filename) == 2);
}

TEST(file_log, flush_on) {
    prepare_logdir();
    std::string filename = "logs/simple_log";

    auto logger = abel_log::create<abel_log::sinks::basic_file_sink_mt>("logger", filename);
    logger->set_pattern("%v");
    logger->set_level(abel_log::level::trace);
    logger->flush_on(abel_log::level::info);
    logger->trace("Should not be flushed");
    EXPECT_TRUE(count_lines(filename) == 0);

    logger->info("Test message {}", 1);
    logger->info("Test message {}", 2);
    logger->flush();
    EXPECT_TRUE(file_contents(filename) == std::string("Should not be flushed\nTest message 1\nTest message 2\n"));
    EXPECT_TRUE(count_lines(filename) == 3);
}

TEST(file_log, rotating_file_logger1) {
    prepare_logdir();
    size_t max_size = 1024 * 10;
    std::string basename = "logs/rotating_log";
    auto logger = abel_log::rotating_logger_mt("logger", basename, max_size, 0);

    for (int i = 0; i < 10; ++i) {
        logger->info("Test message {}", i);
    }

    logger->flush();
    auto filename = basename;
    EXPECT_TRUE(count_lines(filename) == 10);
}

TEST(file_log, rotating_file_logger2) {
    prepare_logdir();
    size_t max_size = 1024 * 10;
    std::string basename = "logs/rotating_log";
    auto logger = abel_log::rotating_logger_mt("logger", basename, max_size, 1);
    for (int i = 0; i < 10; ++i)
        logger->info("Test message {}", i);

    logger->flush();
    auto filename = basename;
    EXPECT_TRUE(count_lines(filename) == 10);
    for (int i = 0; i < 1000; i++) {

        logger->info("Test message {}", i);
    }

    logger->flush();
    EXPECT_TRUE(get_filesize(filename) <= max_size);
    auto filename1 = basename + ".1";
    EXPECT_TRUE(get_filesize(filename1) <= max_size);
}

TEST(file_log, daily_logger_dateonly) {
    using sink_type = abel_log::sinks::daily_file_sink<std::mutex, abel_log::sinks::daily_filename_calculator>;

    prepare_logdir();
    // calculate filename (time based)
    std::string basename = "logs/daily_dateonly";
    std::tm tm = abel::local_tm(abel::now());
    fmt::memory_buffer w;
    fmt::format_to(w, "{}_{:04d}-{:02d}-{:02d}", basename, tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday);

    auto logger = abel_log::create<sink_type>("logger", basename, 0, 0);
    for (int i = 0; i < 10; ++i) {

        logger->info("Test message {}", i);
    }
    logger->flush();
    auto filename = fmt::to_string(w);
    EXPECT_TRUE(count_lines(filename) == 10);
}

struct custom_daily_file_name_calculator {
    static abel_log::filename_t calc_filename (const abel_log::filename_t &basename, const tm &now_tm) {
        fmt::memory_buffer w;
        fmt::format_to(w, "{}{:04d}{:02d}{:02d}", basename, now_tm.tm_year + 1900, now_tm.tm_mon + 1, now_tm.tm_mday);
        return fmt::to_string(w);
    }
};

TEST(file_log, daily_logger_custom) {
    using sink_type = abel_log::sinks::daily_file_sink<std::mutex, custom_daily_file_name_calculator>;

    prepare_logdir();
    // calculate filename (time based)
    std::string basename = "logs/daily_dateonly";
    std::tm tm = abel::local_tm(abel::now());
    fmt::memory_buffer w;
    fmt::format_to(w, "{}{:04d}{:02d}{:02d}", basename, tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday);

    auto logger = abel_log::create<sink_type>("logger", basename, 0, 0);
    for (int i = 0; i < 10; ++i) {
        logger->info("Test message {}", i);
    }

    logger->flush();
    auto filename = fmt::to_string(w);
    EXPECT_TRUE(count_lines(filename) == 10);
}

/*
 * File name calculations
 */

TEST(log_file, filename1) {
    auto filename = abel_log::sinks::rotating_file_sink_st::calc_filename("rotated.txt", 3);
    EXPECT_TRUE(filename == "rotated.3.txt");
}

TEST(log_file, filename2) {
    auto filename = abel_log::sinks::rotating_file_sink_st::calc_filename("rotated", 3);
    EXPECT_TRUE(filename == "rotated.3");
}

TEST(log_file, filename3) {
    auto filename = abel_log::sinks::rotating_file_sink_st::calc_filename("rotated.txt", 0);
    EXPECT_TRUE(filename == "rotated.txt");
}

// regex supported only from gcc 4.9 and above
#if defined(_MSC_VER) || !(__GNUC__ <= 4 && __GNUC_MINOR__ < 9)
#include <regex>

TEST("daily_file_sink::daily_filename_calculator", "[daily_file_sink]]")
{
    // daily_YYYY-MM-DD_hh-mm.txt
    auto filename = abel_log::sinks::daily_filename_calculator::calc_filename("daily.txt", abel_log::details::os::localtime());
    // date regex based on https://www.regular-expressions.info/dates.html
    std::regex re(R"(^daily_(19|20)\d\d-(0[1-9]|1[012])-(0[1-9]|[12][0-9]|3[01])\.txt$)");
    std::smatch match;
    EXPECT_TRUE(std::regex_match(filename, match, re));
}
#endif
