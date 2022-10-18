
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/

#include <iostream>
#include <queue>
#include "testing/gtest_wrap.h"
#include "melon/future/stream_future.h"
#include <array>
#include <queue>
#include <random>


TEST(future_stream, ignored_promise) {
    melon::stream_promise<int> prom;

    (void) prom;
}

TEST(future_stream, ignored_future) {
    melon::stream_future<int> prom;

    (void) prom;
}

TEST(future_stream, forgotten_promise) {
    melon::stream_promise<int> prom;

    auto fut = prom.get_future();

    { auto killer = std::move(prom); }

    auto all_done = fut.for_each([](int) {});

    ASSERT_THROW(all_done.get(), melon::unfull_filled_promise);
}

TEST(future_stream, forgotten_promise_post_bind) {
    melon::stream_promise<int> prom;

    auto fut = prom.get_future();
    auto all_done = fut.for_each([](int) {});

    { auto killer = std::move(prom); }

    ASSERT_THROW(all_done.get(), melon::unfull_filled_promise);
}

TEST(future_stream, forgotten_promise_async) {
    melon::stream_promise<int> prom;

    auto fut = prom.get_future();
    auto all_done = fut.for_each([](int) {});

    std::thread worker([p = std::move(prom)]() {
        std::this_thread::sleep_for(
                std::chrono::duration<double, std::milli>(10.0));
    });

    ASSERT_THROW(all_done.get(), melon::unfull_filled_promise);

    worker.join();
}

TEST(future_stream, simple_stream) {
    melon::stream_promise<int> prom;
    auto fut = prom.get_future();

    int total = 0;

    fut.for_each([&](int v) { total += v; }).finally([&](melon::expected<void>) {
        total = -1;
    });

    ASSERT_EQ(total, 0);

    prom.push(1);
    ASSERT_EQ(total, 1);

    prom.push(2);
    ASSERT_EQ(total, 3);

    prom.push(3);
    ASSERT_EQ(total, 6);

    prom.complete();
    ASSERT_EQ(total, -1);
}

TEST(future_stream, no_data_completed_stream) {
    melon::stream_promise<int> prom;
    auto fut = prom.get_future();

    int total = 0;

    auto done = fut.for_each([&](int v) { total += v; });

    prom.complete();
    done.get();

    ASSERT_EQ(total, 0);
}

TEST(future_stream, no_data_failed_stream) {
    melon::stream_promise<int> prom;
    auto fut = prom.get_future();

    int total = 0;

    auto done = fut.for_each([&](int v) { total += v; });

    prom.set_exception(std::make_exception_ptr(std::runtime_error("")));

    ASSERT_THROW(done.get(), std::runtime_error);

    ASSERT_EQ(total, 0);
}

TEST(future_stream, pre_fill_failure) {
    melon::stream_promise<int> prom;
    auto fut = prom.get_future();

    prom.push(1);
    prom.push(1);
    prom.set_exception(std::make_exception_ptr(std::runtime_error("")));

    int total = 0;
    auto done = fut.for_each([&](int v) { total += v; });

    ASSERT_EQ(total, 2);
    ASSERT_THROW(done.get(), std::runtime_error);
}

TEST(future_stream, partially_failed_stream) {
    melon::stream_promise<int> prom;
    auto fut = prom.get_future();

    int total = 0;
    prom.push(1);
    auto done = fut.for_each([&](int v) { total += v; });

    prom.push(1);
    prom.push(2);
    prom.set_exception(std::make_exception_ptr(std::runtime_error("")));

    ASSERT_THROW(done.get(), std::runtime_error);

    ASSERT_EQ(total, 4);
}

TEST(future_stream, string_stream) {
    melon::stream_promise <std::string> prom;
    auto fut = prom.get_future();

    prom.push("");
    prom.push("");
    prom.push("");

    int total = 0;
    auto done = fut.for_each([&](std::string) -> void { total += 1; });
    prom.push("");
    prom.push("");
    prom.push("");
    ASSERT_EQ(total, 6);

    prom.complete();
    done.get();
}

TEST(future_stream, dynamic_mem_stream) {
    melon::stream_promise <std::unique_ptr<int>> prom;
    auto fut = prom.get_future();

    prom.push(std::make_unique<int>(1));
    prom.push(std::make_unique<int>(1));
    prom.push(std::make_unique<int>(1));

    int total = 0;
    auto done = fut.for_each([&](std::unique_ptr<int> v) { total += *v; });

    prom.push(std::make_unique<int>(1));
    prom.push(std::make_unique<int>(1));
    prom.push(std::make_unique<int>(1));
    ASSERT_EQ(total, 6);

    prom.complete();
    done.get();
}

TEST(future_stream, dynamic_mem_dropped) {
    melon::stream_promise <std::unique_ptr<int>> prom;
    auto fut = prom.get_future();

    prom.push(std::make_unique<int>(1));
    prom.push(std::make_unique<int>(1));
    prom.push(std::make_unique<int>(1));

    int total = 0;
    auto done = fut.for_each([&](auto v) { total += *v; });
    prom.push(std::make_unique<int>(1));
    prom.push(std::make_unique<int>(1));
    prom.push(std::make_unique<int>(1));
    ASSERT_EQ(total, 6);

    { auto killer = std::move(prom); }

    ASSERT_THROW(done.get(), melon::unfull_filled_promise);
}

TEST(future_stream, multiple_pre_filled) {
    melon::stream_promise<int> prom;
    auto fut = prom.get_future();

    prom.push(1);
    prom.push(2);
    int total = 0;
    auto done = fut.for_each([&](int v) { total += v; });

    ASSERT_EQ(total, 3);

    prom.complete();
    done.get();
}

TEST(future_stream, uncompleted_stream) {
    melon::stream_promise<int> prom;
    auto fut = prom.get_future();

    int total = 0;

    auto done = fut.for_each([&](int v) { total += v; });

    {
        melon::stream_promise<int> destroyer = std::move(prom);
        ASSERT_EQ(total, 0);
        destroyer.push(1);
        ASSERT_EQ(total, 1);
        destroyer.push(2);
        ASSERT_EQ(total, 3);
    }

    ASSERT_THROW(done.get(), melon::unfull_filled_promise);
}

TEST(future_stream, mt_random_timing) {
    std::random_device rd;
    std::mt19937 e2(rd());
    std::uniform_real_distribution<> dist(0, 0.002);

    melon::stream_promise<int> prom;
    auto fut = prom.get_future();

    std::thread worker([&]() {
        for (int i = 0; i < 10000; ++i) {
            std::this_thread::sleep_for(
                    std::chrono::duration<double, std::milli>(dist(e2)));
            prom.push(1);
        }
        prom.complete();
    });

    int total = 0;

    auto done = fut.for_each([&](int v) { total += v; });

    done.get();
    ASSERT_EQ(total, 10000);
    worker.join();
}

TEST(future_stream, delayed_assignment) {
    melon::stream_promise<int> prom;
    auto fut = prom.get_future();

    int total = 0;

    {
        melon::stream_promise<int> destroyer = std::move(prom);
        ASSERT_EQ(total, 0);
        destroyer.push(1);
        ASSERT_EQ(total, 0);
        destroyer.push(2);
        ASSERT_EQ(total, 0);
        destroyer.complete();
        ASSERT_EQ(total, 0);
    }

    auto done = fut.for_each([&](int v) { total += v; });

    done.get();
    ASSERT_EQ(total, 3);
}

TEST(future_stream, stream_to_queue) {
    std::queue<std::function<void()>> queue;

    melon::stream_promise<int> prom;
    auto fut = prom.get_future();
    int total = 0;
    bool all_done = false;

    fut.for_each(queue, [&](int v) { total += v; }).finally([&](melon::expected<void>) {
        all_done = true;
    });

    prom.push(1);
    prom.push(1);
    prom.push(1);
    prom.complete();

    ASSERT_EQ(0, total);
    ASSERT_EQ(queue.size(), 4);
    ASSERT_FALSE(all_done);

    while (!queue.empty()) {
        queue.front()();
        queue.pop();
    }

    ASSERT_EQ(3, total);
    ASSERT_TRUE(all_done);
}

TEST(future_stream, stream_to_queue_alt) {
    std::queue<std::function<void()>> queue;

    melon::stream_promise<int> prom;
    auto fut = prom.get_future();
    int total = 0;
    bool all_done = false;

    prom.push(1);
    prom.push(1);
    prom.push(1);

    ASSERT_EQ(queue.size(), 0);

    fut.for_each(queue, [&](int v) { total += v; }).finally([&](melon::expected<void>) {
        all_done = true;
    });

    ASSERT_EQ(queue.size(), 3);

    prom.complete();

    ASSERT_EQ(0, total);
    ASSERT_EQ(queue.size(), 4);
    ASSERT_FALSE(all_done);

    while (!queue.empty()) {
        queue.front()();
        queue.pop();
    }

    ASSERT_EQ(3, total);
    ASSERT_TRUE(all_done);
}

struct Synced_queue {
    void push(std::function<void()> p) {
        std::lock_guard l(mtx_);
        queue.push(std::move(p));
    }

    bool pop() {
        std::lock_guard l(mtx_);
        if (queue.empty()) {
            return true;
        }
        queue.front()();
        queue.pop();
        return false;
    }

    std::queue<std::function<void()>> queue;
    std::mutex mtx_;
};

TEST(future_stream, stream_to_queue_random_timing) {
    std::random_device rd;
    std::mt19937 e2(rd());
    std::uniform_real_distribution<> dist(0, 0.002);

    Synced_queue queue;

    melon::stream_promise<int> prom;
    auto fut = prom.get_future();

    int total = 0;
    bool all_done = false;

    std::thread pusher([&]() {
        for (int i = 0; i < 10000; ++i) {
            std::this_thread::sleep_for(
                    std::chrono::duration<double, std::milli>(dist(e2)));
            prom.push(1);
        }
        prom.complete();
    });

    std::this_thread::sleep_for(std::chrono::duration<double, std::milli>(50));
    fut.for_each(queue, [&](int v) { total += v; }).finally([&](melon::expected<void>) {
        all_done = true;
    });

    std::this_thread::sleep_for(std::chrono::duration<double, std::milli>(50));
    while (!queue.pop()) {
    }

    pusher.join();
    while (!queue.pop()) {
    }

    ASSERT_EQ(10000, total);
    ASSERT_TRUE(all_done);
}
