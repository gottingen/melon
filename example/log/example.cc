
#include <iostream>
#include "abel/log/logging.h"

void abel_log();
void stdout_example();

void basic_example();

void rotating_example();

void daily_example();

void async_example();

void multi_sink_example();

void user_defined_example();

void err_handler_example();

void syslog_example();

#include "abel/log/log.h"

int main(int, char *[]) {

    try {
        abel_log();
        // console logging example
        stdout_example();

        // various file loggers
        basic_example();
        rotating_example();
        daily_example();

        // async logging using a backing thread pool
        async_example();

        // a logger can have multiple targets with different formats
        multi_sink_example();


        // flush all *registered* loggers using a worker thread every 3 seconds.
        // note: registered loggers *must* be thread safe for this to work correctly!
        abel::flush_every(std::chrono::seconds(3));

        // apply some function on all registered loggers
        abel::apply_all([&](std::shared_ptr<abel::logger> l) { l->info("End of example."); });

        abel::shutdown();
    }
        // Exceptions will only be thrown upon failed logger or sink construction (not during logging)
    catch (const abel::log_ex &ex) {
        std::cout << "Log init failed: " << ex.what() << std::endl;
        return 1;
    }
}

#include "abel/log/sinks/stdout_color_sinks.h"

void stdout_example() {
    // create color multi threaded logger
    auto console = abel::stdout_color_mt("console");
    console->error("Some error message with arg: {}", 1);

    auto err_logger = abel::stderr_color_mt("stderr");
    err_logger->error("Some error message");

    // Formatting examples
    console->warn("Easy padding in numbers like {:08d}", 12);
    console->critical("Support for int: {0:d};  hex: {0:x};  oct: {0:o}; bin: {0:b}", 42);
    console->info("Support for floats {:03.2f}", 1.23456);
    console->info("Positional args are {1} {0}..", "too", "supported");
    console->info("{:<30}", "left aligned");

    abel::get("console")->info("loggers can be retrieved from a global registry using the abel::get(logger_name)");

    // Runtime log levels
    abel::set_level(abel::level::info); // Set global log level to info
    console->debug("This message should not be displayed!");
    console->set_level(abel::level::trace); // Set specific logger's log level
    console->debug("This message should be displayed..");

    // Customize msg format for all loggers
    abel::set_pattern("[%H:%M:%S %z] [%n] [%^---%L---%$] [thread %t] %v");
    console->info("This an info message with custom format");

    // Compile time log levels
    // define LOG_DEBUG_ON or LOG_TRACE_ON
    LOG_INFO(console, "this should display {} ,{}", 1, 3.23);
    LOG_IF_INFO(console, false, "this should not diplay {} ,{}", 1, 3.23);
    LOG_IF_INFO(console, true, "this should diplay LOG_IF_INFO {} ,{}", 1, 3.23);

    for(int i=0; i < 200; i++) {
        LOG_CALL_IF_EVERY_N(console, abel::level::info, true, 100,  "this should display twice {} ,{}", 1, 3.23);
    }
}

#include "abel/log/sinks/basic_file_sink.h"

void basic_example() {
    // Create basic file logger (not rotated)
    auto my_logger = abel::basic_logger_mt("basic_logger", "logs/basic-log.txt");
}

#include "abel/log/sinks/rotating_file_sink.h"

void rotating_example() {
    // Create a file rotating logger with 5mb size max and 3 rotated files
    auto rotating_logger = abel::rotating_logger_mt("some_logger_name", "logs/rotating.txt", 1048576 * 5, 3);
}

#include "abel/log/sinks/daily_file_sink.h"

void daily_example() {
    // Create a daily logger - a new file is created every day on 2:30am
    auto daily_logger = abel::daily_logger_mt("daily_logger", "logs/daily.txt", 2, 30);
}

#include "abel/log/async.h"

void async_example() {
    // default thread pool settings can be modified *before* creating the async logger:
    // abel::init_thread_pool(32768, 1); // queue with max 32k items 1 backing thread.
    auto async_file = abel::basic_logger_mt<abel::async_factory>("async_file_logger", "logs/async_log.txt");
    // alternatively:
    // auto async_file = abel::create_async<abel::sinks::basic_file_sink_mt>("async_file_logger", "logs/async_log.txt");

    for (int i = 1; i < 101; ++i) {
        async_file->info("Async message #{}", i);
    }
}

// create logger with 2 targets with different log levels and formats
// the console will show only warnings or errors, while the file will log all

void multi_sink_example() {
    auto console_sink = std::make_shared<abel::sinks::stdout_color_sink_mt>();
    console_sink->set_level(abel::level::warn);
    console_sink->set_pattern("[multi_sink_example] [%^%l%$] %v");

    auto file_sink = std::make_shared<abel::sinks::basic_file_sink_mt>("logs/multisink.txt", true);
    file_sink->set_level(abel::level::trace);

    abel::logger logger("multi_sink", {console_sink, file_sink});
    logger.set_level(abel::level::debug);
    logger.warn("this should appear in both console and file");
    logger.info("this message should not appear in the console, only in the file");
}

void abel_log() {
    DLOG_TRACE("this is trace");
    DLOG_DEBUG("this is debug");
    DLOG_INFO("this is info");
    DLOG_WARN("this is warn");
    DLOG_ERROR("this is error");
    DCHECK_MSG(true,"abc");
}
