#include <test/testing/log_includes.h>
#include <abel/log/async.h>
#include <abel/log/sinks/basic_file_sink.h>
#include "sink_test.h"
#include <gtest/gtest.h>

TEST(BasicAsync, async) {
    using namespace abel;
    auto test_sink = std::make_shared<sinks::test_sink_mt>();
    size_t queue_size = 128;
    size_t messages = 256;
    {
        auto tp = std::make_shared<details::thread_pool>(queue_size, 1);
        auto logger = std::make_shared<async_logger>("as", test_sink, tp, async_overflow_policy::block);
        for (size_t i = 0; i < messages; i++) {
            logger->info("Hello message #{}", i);
        }
        logger->flush();
    }
    EXPECT_TRUE(test_sink->msg_counter() == messages);
    EXPECT_TRUE(test_sink->flush_counter() == 1);
}

TEST(discardpolicy, async) {
    using namespace abel;
    auto test_sink = std::make_shared<sinks::test_sink_mt>();
    size_t queue_size = 2;
    size_t messages = 10240;

    auto tp = std::make_shared<details::thread_pool>(queue_size, 1);
    auto logger = std::make_shared<async_logger>("as", test_sink, tp, async_overflow_policy::overrun_oldest);
    for (size_t i = 0; i < messages; i++) {
        logger->info("Hello message");
    }
    EXPECT_TRUE(test_sink->msg_counter() < messages);
}

TEST(DiscarPpolicy, async
) {
    using namespace abel;
    size_t queue_size = 2;
    size_t messages = 10240;
    abel::init_thread_pool(queue_size,
                             1);

    auto logger = abel::create_async_nb<sinks::test_sink_mt>("as2");
    for (
        size_t i = 0;
        i < messages;
        i++) {
        logger->info("Hello message");
    }
    auto sink = std::static_pointer_cast<sinks::test_sink_mt>(logger->sinks()[0]);
    EXPECT_TRUE(sink->msg_counter() < messages);
    abel::drop_all();
}

TEST(flush, async) {
    using namespace abel;
    auto test_sink = std::make_shared<sinks::test_sink_mt>();
    size_t queue_size = 256;
    size_t messages = 256;
    {
        auto tp = std::make_shared<details::thread_pool>(queue_size, 1);
        auto logger = std::make_shared<async_logger>("as", test_sink, tp, async_overflow_policy::block);
        for (size_t i = 0; i < messages; i++) {
            logger->info("Hello message #{}", i);
        }

        logger->flush();
    }
    // std::this_thread::sleep_for(std::chrono::milliseconds(250));
    EXPECT_TRUE(test_sink->msg_counter() == messages);
    EXPECT_TRUE(test_sink->flush_counter() == 1);
}

TEST(AsyncPeriodicFlush, async) {
    using namespace abel;

    auto logger = abel::create_async<sinks::test_sink_mt>("as");

    auto test_sink = std::static_pointer_cast<sinks::test_sink_mt>(logger->sinks()[0]);

    abel::flush_every(std::chrono::seconds(1));
    std::this_thread::sleep_for(std::chrono::milliseconds(1100));
    EXPECT_TRUE(test_sink->flush_counter() == 1);
    abel::flush_every(std::chrono::seconds(0));
    abel::drop_all();
}

TEST(wait_empty, async) {
    using namespace abel;
    auto test_sink = std::make_shared<sinks::test_sink_mt>();
    test_sink->set_delay(std::chrono::milliseconds(5));
    size_t messages = 100;

    auto tp = std::make_shared<details::thread_pool>(messages, 2);
    auto logger = std::make_shared<async_logger>("as", test_sink, tp, async_overflow_policy::block);
    for (size_t i = 0; i < messages; i++) {
        logger->info("Hello message #{}", i);
    }
    logger->flush();
    tp.reset();

    EXPECT_TRUE(test_sink->msg_counter() == messages);
    EXPECT_TRUE(test_sink->flush_counter() == 1);
}

TEST(multithreads, async) {
    using namespace abel;
    auto test_sink = std::make_shared<sinks::test_sink_mt>();
    size_t queue_size = 128;
    size_t messages = 256;
    size_t n_threads = 10;
    {
        auto tp = std::make_shared<details::thread_pool>(queue_size, 1);
        auto logger = std::make_shared<async_logger>("as", test_sink, tp, async_overflow_policy::block);

        std::vector<std::thread> threads;
        for (size_t i = 0; i < n_threads; i++) {
            threads.emplace_back([logger, messages] {
                for (size_t j = 0; j < messages; j++) {
                    logger->info("Hello message #{}", j);
                }
            });
            logger->flush();
        }

        for (auto &t : threads) {
            t.join();
        }
    }

    EXPECT_TRUE(test_sink->msg_counter() == messages * n_threads);
    EXPECT_TRUE(test_sink->flush_counter() == n_threads);
}

TEST(ToFile, async) {
    prepare_logdir();
    size_t messages = 1024;
    size_t tp_threads = 1;
    std::string filename = "logs/async_test.log";
    {
        auto file_sink = std::make_shared<abel::sinks::basic_file_sink_mt>(filename, true);
        auto tp = std::make_shared<abel::details::thread_pool>(messages, tp_threads);
        auto logger = std::make_shared<abel::async_logger>("as", std::move(file_sink), std::move(tp));

        for (size_t j = 0; j < messages; j++) {
            logger->info("Hello message #{}", j);
        }
    }

    EXPECT_TRUE(count_lines(filename) == messages);
    auto contents = file_contents(filename);
    EXPECT_TRUE(ends_with(contents, std::string("Hello message #1023\n")));
}

TEST(multi_workers, async) {
    prepare_logdir();
    size_t messages = 1024 * 10;
    size_t tp_threads = 10;
    std::string filename = "logs/async_test.log";
    {
        auto file_sink = std::make_shared<abel::sinks::basic_file_sink_mt>(filename, true);
        auto tp = std::make_shared<abel::details::thread_pool>(messages, tp_threads);
        auto logger = std::make_shared<abel::async_logger>("as", std::move(file_sink), std::move(tp));

        for (size_t j = 0; j < messages; j++) {
            logger->info("Hello message #{}", j);
        }
    }

    EXPECT_TRUE(count_lines(filename) == messages);
}
