
#include <test/testing/log_includes.h>

TEST(FileLog, simplefilelogger) {
    prepare_logdir();
    std::string filename = "logs/simple_log";

    auto logger = abel::log::create<abel::log::sinks::basic_file_sink_mt>("logger", filename);
    logger->set_pattern("%v");

    logger->info("Test message {}", 1);
    logger->info("Test message {}", 2);

    logger->flush();
    EXPECT_TRUE(file_contents(filename) == std::string("Test message 1\nTest message 2\n"));
    EXPECT_TRUE(count_lines(filename) == 2);
}

TEST(FileLog, flush_on) {
    prepare_logdir();
    std::string filename = "logs/simple_log";

    auto logger = abel::log::create<abel::log::sinks::basic_file_sink_mt>("logger", filename);
    logger->set_pattern("%v");
    logger->set_level(abel::log::trace);
    logger->flush_on(abel::log::info);
    logger->trace("Should not be flushed");
    EXPECT_TRUE(count_lines(filename) == 0);

    logger->info("Test message {}", 1);
    logger->info("Test message {}", 2);
    logger->flush();
    EXPECT_TRUE(file_contents(filename) == std::string("Should not be flushed\nTest message 1\nTest message 2\n"));
    EXPECT_TRUE(count_lines(filename) == 3);
}

TEST(FileLog, rotatingfilelogger1) {
    prepare_logdir();
    size_t max_size = 1024 * 10;
    std::string basename = "logs/rotating_log";
    auto logger = abel::log::rotating_logger_mt("logger", basename, max_size, 0);

    for (int i = 0; i < 10; ++i) {
        logger->info("Test message {}", i);
    }

    logger->flush();
    auto filename = basename;
    EXPECT_TRUE(count_lines(filename) == 10);
}

TEST(FileLog, rotatingfilelogger2) {
    prepare_logdir();
    size_t max_size = 1024 * 10;
    std::string basename = "logs/rotating_log";
    auto logger = abel::log::rotating_logger_mt("logger", basename, max_size, 1);
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

TEST(FileLog, dailyloggerdateonly) {
    using sink_type = abel::log::sinks::daily_file_sink<std::mutex, abel::log::sinks::daily_filename_calculator>;

    prepare_logdir();
    // calculate filename (time based)
    std::string basename = "logs/daily_dateonly";
    std::tm tm = abel::local_tm(abel::now());
    fmt::memory_buffer w;
    fmt::format_to(w, "{}_{:04d}-{:02d}-{:02d}", basename, tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday);

    auto logger = abel::log::create<sink_type>("logger", basename, 0, 0);
    for (int i = 0; i < 10; ++i) {

        logger->info("Test message {}", i);
    }
    logger->flush();
    auto filename = fmt::to_string(w);
    EXPECT_TRUE(count_lines(filename) == 10);
}

struct custom_daily_file_name_calculator {
    static abel::log::filename_t calc_filename(const abel::log::filename_t &basename, const tm &now_tm) {
        fmt::memory_buffer w;
        fmt::format_to(w, "{}{:04d}{:02d}{:02d}", basename, now_tm.tm_year + 1900, now_tm.tm_mon + 1, now_tm.tm_mday);
        return fmt::to_string(w);
    }
};

TEST(FileLog, dailyloggercustom) {
    using sink_type = abel::log::sinks::daily_file_sink<std::mutex, custom_daily_file_name_calculator>;

    prepare_logdir();
    // calculate filename (time based)
    std::string basename = "logs/daily_dateonly";
    std::tm tm = abel::local_tm(abel::now());
    fmt::memory_buffer w;
    fmt::format_to(w, "{}{:04d}{:02d}{:02d}", basename, tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday);

    auto logger = abel::log::create<sink_type>("logger", basename, 0, 0);
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

TEST(LogFile, filename1) {
    auto filename = abel::log::sinks::rotating_file_sink_st::calc_filename("rotated.txt", 3);
    EXPECT_TRUE(filename == "rotated.3.txt");
}

TEST(LogFile, filename2) {
    auto filename = abel::log::sinks::rotating_file_sink_st::calc_filename("rotated", 3);
    EXPECT_TRUE(filename == "rotated.3");
}

TEST(LogFile, filename3) {
    auto filename = abel::log::sinks::rotating_file_sink_st::calc_filename("rotated.txt", 0);
    EXPECT_TRUE(filename == "rotated.txt");
}
/*
// regex supported only from gcc 4.9 and above
#if defined(_MSC_VER) || !(__GNUC__ <= 4 && __GNUC_MINOR__ < 9)
#include <regex>

TEST(dailyfilesink, daily_file_sink)
{
    // daily_YYYY-MM-DD_hh-mm.txt
    auto filename = abel::log::sinks::daily_filename_calculator::calc_filename("daily.txt", abel::log::local_tm(abel::log::now()));
    // date regex based on https://www.regular-expressions.info/dates.html
    std::regex re(R"(^daily_(19|20)\d\d-(0[1-9]|1[012])-(0[1-9]|[12][0-9]|3[01])\.txt$)");
    std::smatch match;
    EXPECT_TRUE(std::regex_match(filename, match, re));
}

#endif
 */
