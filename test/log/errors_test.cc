
#include <test/testing/log_includes.h>
#include <iostream>

class failing_sink : public abel::sinks::base_sink<std::mutex> {
public:
    failing_sink () = default;
    ~failing_sink () = default;

protected:
    void sink_it_ (const abel::details::log_msg &) override {
        throw std::runtime_error("some error happened during log");
    }

    void flush_ () override {
        throw std::runtime_error("some error happened during flush");
    }
};

TEST(default_error_handler, errors) {

    prepare_logdir();
    std::string filename = "logs/simple_log.txt";

    auto logger = abel::create<abel::sinks::basic_file_sink_mt>("test-error", filename, true);
    logger->set_pattern("%v");
    logger->info("Test message {} {}", 1);
    logger->info("Test message {}", 2);
    logger->
        flush();

    EXPECT_TRUE(file_contents(filename)
                    == std::string("Test message 2\n"));
    EXPECT_TRUE(count_lines(filename)
                    == 1);
}

struct custom_ex {
};
TEST(errors, custom_error_handler) {
    prepare_logdir();
    std::string filename = "logs/simple_log.txt";
    auto logger = abel::create<abel::sinks::basic_file_sink_mt>("logger", filename, true);
    logger->
        flush_on(abel::level::info);
    logger->set_error_handler([=] (const std::string &) {
        throw
            custom_ex();
    });
    logger->info("Good message #1");

    EXPECT_THROW(logger
                     ->info("Bad format msg {} {}", "xxx"), custom_ex);
    logger->info("Good message #2");
    EXPECT_TRUE(count_lines(filename)
                    == 2);
}

TEST(errors, default_error_handler2) {
    abel::drop_all();
    auto logger = abel::create<failing_sink>("failed_logger");
    logger->set_error_handler([=] (const std::string &) {
        throw
            custom_ex();
    });
    EXPECT_THROW(logger
                     ->info("Some message"), custom_ex);
}

TEST(errors, flush_error_handler) {
    abel::drop_all();
    auto logger = abel::create<failing_sink>("failed_logger");
    logger->set_error_handler([=] (const std::string &) {
        throw
            custom_ex();
    });
    EXPECT_THROW(logger->flush(), custom_ex
    );
}

TEST(errors, async_error_handler) {
    prepare_logdir();
    std::string err_msg("log failed with some msg");

    std::string filename = "logs/simple_async_log.txt";
    {
        abel::init_thread_pool(128, 1);
        auto logger = abel::create_async<abel::sinks::basic_file_sink_mt>("logger", filename, true);
        logger->set_error_handler([=] (const std::string &) {
            std::ofstream ofs("logs/custom_err.txt");
            if (!ofs)
                throw std::runtime_error("Failed open logs/custom_err.txt");
            ofs <<
                err_msg;
        });
        logger->info("Good message #1");
        logger->info("Bad format msg {} {}", "xxx");
        logger->info("Good message #2");
        abel::drop("logger"); // force logger to drain the queue and shutdown
    }
    abel::init_thread_pool(128, 1);
    EXPECT_TRUE(count_lines(filename)
                    == 2);
    EXPECT_TRUE(file_contents("logs/custom_err.txt")
                    == err_msg);
}

// Make sure async error handler is executed
TEST(errors, async_error_handler2) {
    prepare_logdir();
    std::string err_msg("This is async handler error message");
    {
        abel::init_thread_pool(128, 1);
        auto logger = abel::create_async<failing_sink>("failed_logger");
        logger->set_error_handler([=] (const std::string &) {
            std::ofstream ofs("logs/custom_err2.txt");
            if (!ofs)
                throw std::runtime_error("Failed open logs/custom_err2.txt");
            ofs <<
                err_msg;
        });
        logger->info("Hello failure");
        abel::drop("failed_logger"); // force logger to drain the queue and shutdown
    }

    abel::init_thread_pool(128, 1);
    EXPECT_TRUE(file_contents("logs/custom_err2.txt")
                    == err_msg);
}
