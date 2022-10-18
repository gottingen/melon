
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/

#include <iostream>
#include "testing/gtest_wrap.h"
#include "melon/future/future.h"
#include <array>
#include <queue>
#include <random>

namespace {

    struct prom_fut {
        prom_fut() { f = p.get_future(); }

        int get() { return f.std_future().get(); }

        melon::promise<int> p;
        melon::future<int> f;
    };

    struct pf_set {
        prom_fut pf[4];

        prom_fut &operator[](std::size_t i) { return pf[i]; }

        void complete() {
            pf[0].p.set_value(1);
            pf[1].p.set_exception(std::make_exception_ptr(std::logic_error("nope")));
            pf[2].p.finish(melon::expected<int>(1));
            pf[3].p.finish(
                    melon::unexpected(std::make_exception_ptr(std::logic_error(""))));
        }
    };

    void no_op(int i) { ASSERT_EQ(i, 1); }

    void failure(int i) {
        ASSERT_EQ(i, 1);
        throw std::runtime_error("dead");
    }

    int expect_noop_count = 0;

    int expected_noop(melon::expected<int>) {
        ++expect_noop_count;
        return 1;
    }

    void expected_noop_fail(melon::expected<int>) { throw std::runtime_error("dead"); }


    melon::expected<int> generate_expected_value(int) { return 3; }

    melon::expected<int> generate_expected_value_fail(int) {
        return melon::unexpected{std::make_exception_ptr(std::runtime_error("yo"))};
    }

    melon::expected<int> generate_expected_value_throw(int) {
        throw std::runtime_error("yo");
    }


    melon::expected<int> te_generate_expected_value(melon::expected<int>) { return 3; }

    melon::expected<int> te_generate_expected_value_fail(melon::expected<int>) {
        return melon::unexpected{std::make_exception_ptr(std::runtime_error("yo"))};
    }

    melon::expected<int> te_generate_expected_value_throw(melon::expected<int>) {
        throw std::runtime_error("yo");
    }

}  // namespace


TEST(future_int, blank) { melon::future<int> fut; }

TEST(future_int, unfilled_promise_failiure) {
    melon::future<int> fut;
    {
        melon::promise<int> p;
        fut = p.get_future();
    }

    ASSERT_THROW(fut.std_future().get(), melon::unfull_filled_promise);
}

TEST(future_int, preloaded_std_get) {
    pf_set pf;

    pf.complete();

    ASSERT_EQ(1, pf[0].get());
    ASSERT_THROW(pf[1].get(), std::logic_error);
    ASSERT_EQ(1, pf[2].get());
    ASSERT_THROW(pf[3].get(), std::logic_error);
}

TEST(future_int, delayed_std_get) {
    pf_set pf;

    std::mutex mtx;
    std::unique_lock l(mtx);
    std::thread thread([&] {
        std::lock_guard ll(mtx);
        pf.complete();
    });

    auto std_f1 = pf[0].f.std_future();
    auto std_f2 = pf[1].f.std_future();
    auto std_f3 = pf[2].f.std_future();
    auto std_f4 = pf[3].f.std_future();

    l.unlock();

    ASSERT_EQ(1, std_f1.get());
    ASSERT_THROW(std_f2.get(), std::logic_error);
    ASSERT_EQ(1, std_f3.get());
    ASSERT_THROW(std_f4.get(), std::logic_error);

    thread.join();
}

TEST(future_int, then_noop_pre) {
    pf_set pf;

    auto f1 = pf[0].f.then(no_op);
    auto f2 = pf[1].f.then(no_op);
    auto f3 = pf[2].f.then(no_op);
    auto f4 = pf[3].f.then(no_op);

    pf.complete();

    ASSERT_NO_THROW(f1.std_future().get());
    ASSERT_THROW(f2.std_future().get(), std::logic_error);
    ASSERT_NO_THROW(f3.std_future().get());
    ASSERT_THROW(f4.std_future().get(), std::logic_error);
}

TEST(future_int, then_noop_post) {
    pf_set pf;

    pf.complete();

    auto f1 = pf[0].f.then(no_op);
    auto f2 = pf[1].f.then(no_op);
    auto f3 = pf[2].f.then(no_op);
    auto f4 = pf[3].f.then(no_op);

    ASSERT_NO_THROW(f1.std_future().get());
    ASSERT_THROW(f2.std_future().get(), std::logic_error);
    ASSERT_NO_THROW(f3.std_future().get());
    ASSERT_THROW(f4.std_future().get(), std::logic_error);
}

TEST(future_int, then_failure_pre) {
    pf_set pf;

    auto f1 = pf[0].f.then(failure);
    auto f2 = pf[1].f.then(failure);
    auto f3 = pf[2].f.then(failure);
    auto f4 = pf[3].f.then(failure);

    pf.complete();

    ASSERT_THROW(f1.std_future().get(), std::runtime_error);
    ASSERT_THROW(f2.std_future().get(), std::logic_error);
    ASSERT_THROW(f3.std_future().get(), std::runtime_error);
    ASSERT_THROW(f4.std_future().get(), std::logic_error);
}

TEST(future_int, then_failure_post) {
    pf_set pf;

    pf.complete();

    auto f1 = pf[0].f.then(failure);
    auto f2 = pf[1].f.then(failure);
    auto f3 = pf[2].f.then(failure);
    auto f4 = pf[3].f.then(failure);

    ASSERT_THROW(f1.std_future().get(), std::runtime_error);
    ASSERT_THROW(f2.std_future().get(), std::logic_error);
    ASSERT_THROW(f3.std_future().get(), std::runtime_error);
    ASSERT_THROW(f4.std_future().get(), std::logic_error);
}

TEST(future_int, then_expect_success_pre) {
    pf_set pf;

    auto f1 = pf[0].f.then_expect(expected_noop);
    auto f2 = pf[1].f.then_expect(expected_noop);
    auto f3 = pf[2].f.then_expect(expected_noop);
    auto f4 = pf[3].f.then_expect(expected_noop);

    pf.complete();

    ASSERT_EQ(1, f1.std_future().get());
    ASSERT_EQ(1, f2.std_future().get());
    ASSERT_EQ(1, f3.std_future().get());
    ASSERT_EQ(1, f4.std_future().get());
}

TEST(future_int, then_expect_success_post) {
    pf_set pf;

    auto f1 = pf[0].f.then_expect(expected_noop);
    auto f2 = pf[1].f.then_expect(expected_noop);
    auto f3 = pf[2].f.then_expect(expected_noop);
    auto f4 = pf[3].f.then_expect(expected_noop);

    pf.complete();

    ASSERT_EQ(1, f1.std_future().get());
    ASSERT_EQ(1, f2.std_future().get());
    ASSERT_EQ(1, f3.std_future().get());
    ASSERT_EQ(1, f4.std_future().get());
}

TEST(future_int, then_expect_failure_pre) {
    pf_set pf;

    pf.complete();

    auto f1 = pf[0].f.then_expect(expected_noop_fail);
    auto f2 = pf[1].f.then_expect(expected_noop_fail);
    auto f3 = pf[2].f.then_expect(expected_noop_fail);
    auto f4 = pf[3].f.then_expect(expected_noop_fail);

    ASSERT_THROW(f1.std_future().get(), std::runtime_error);
    ASSERT_THROW(f2.std_future().get(), std::runtime_error);
    ASSERT_THROW(f3.std_future().get(), std::runtime_error);
    ASSERT_THROW(f4.std_future().get(), std::runtime_error);
}

TEST(future_int, then_expect_failure_post) {
    pf_set pf;

    auto f1 = pf[0].f.then_expect(expected_noop_fail);
    auto f2 = pf[1].f.then_expect(expected_noop_fail);
    auto f3 = pf[2].f.then_expect(expected_noop_fail);
    auto f4 = pf[3].f.then_expect(expected_noop_fail);

    pf.complete();

    ASSERT_THROW(f1.std_future().get(), std::runtime_error);
    ASSERT_THROW(f2.std_future().get(), std::runtime_error);
    ASSERT_THROW(f3.std_future().get(), std::runtime_error);
    ASSERT_THROW(f4.std_future().get(), std::runtime_error);
}

TEST(future_int, then_expect_finally_success_pre) {
    pf_set pf;

    auto ref = expect_noop_count;

    pf[0].f.finally(expected_noop);
    pf[1].f.finally(expected_noop);
    pf[2].f.finally(expected_noop);
    pf[3].f.finally(expected_noop);

    pf.complete();

    ASSERT_EQ(4 + ref, expect_noop_count);
}

TEST(future_int, then_expect_finally_success_post) {
    pf_set pf;

    auto ref = expect_noop_count;

    pf.complete();

    pf[0].f.finally(expected_noop);
    pf[1].f.finally(expected_noop);
    pf[2].f.finally(expected_noop);
    pf[3].f.finally(expected_noop);

    ASSERT_EQ(4 + ref, expect_noop_count);
}


TEST(future_int, expected_returning_callback) {
    pf_set pf;

    pf.complete();

    auto f1 = pf[0].f.then(generate_expected_value);
    auto f2 = pf[1].f.then(generate_expected_value);
    auto f3 = pf[2].f.then(generate_expected_value);
    auto f4 = pf[3].f.then(generate_expected_value);

    ASSERT_EQ(3, f1.get());
    ASSERT_THROW(f2.std_future().get(), std::logic_error);
    ASSERT_EQ(3, f3.get());
    ASSERT_THROW(f4.std_future().get(), std::logic_error);
}

TEST(future_int, expected_returning_callback_fail) {
    pf_set pf;

    pf.complete();

    auto f1 = pf[0].f.then(generate_expected_value_fail);
    auto f2 = pf[1].f.then(generate_expected_value_fail);
    auto f3 = pf[2].f.then(generate_expected_value_fail);
    auto f4 = pf[3].f.then(generate_expected_value_fail);

    ASSERT_THROW(f1.std_future().get(), std::runtime_error);
    ASSERT_THROW(f2.std_future().get(), std::logic_error);
    ASSERT_THROW(f3.std_future().get(), std::runtime_error);
    ASSERT_THROW(f4.std_future().get(), std::logic_error);
}

TEST(future_int, expected_returning_callback_throw) {
    pf_set pf;

    pf.complete();

    auto f1 = pf[0].f.then(generate_expected_value_throw);
    auto f2 = pf[1].f.then(generate_expected_value_throw);
    auto f3 = pf[2].f.then(generate_expected_value_throw);
    auto f4 = pf[3].f.then(generate_expected_value_throw);

    ASSERT_THROW(f1.std_future().get(), std::runtime_error);
    ASSERT_THROW(f2.std_future().get(), std::logic_error);
    ASSERT_THROW(f3.std_future().get(), std::runtime_error);
    ASSERT_THROW(f4.std_future().get(), std::logic_error);
}


TEST(future_int, te_expected_returning_callback) {
    pf_set pf;

    pf.complete();

    auto f1 = pf[0].f.then_expect(te_generate_expected_value);
    auto f2 = pf[1].f.then_expect(te_generate_expected_value);
    auto f3 = pf[2].f.then_expect(te_generate_expected_value);
    auto f4 = pf[3].f.then_expect(te_generate_expected_value);

    ASSERT_EQ(3, f1.get());
    ASSERT_EQ(3, f2.get());
    ASSERT_EQ(3, f3.get());
    ASSERT_EQ(3, f4.get());
}

TEST(future_int, te_expected_returning_callback_fail) {
    pf_set pf;

    pf.complete();

    auto f1 = pf[0].f.then_expect(te_generate_expected_value_fail);
    auto f2 = pf[1].f.then_expect(te_generate_expected_value_fail);
    auto f3 = pf[2].f.then_expect(te_generate_expected_value_fail);
    auto f4 = pf[3].f.then_expect(te_generate_expected_value_fail);

    ASSERT_THROW(f1.std_future().get(), std::runtime_error);
    ASSERT_THROW(f2.std_future().get(), std::runtime_error);
    ASSERT_THROW(f3.std_future().get(), std::runtime_error);
    ASSERT_THROW(f4.std_future().get(), std::runtime_error);
}

TEST(future_int, te_expected_returning_callback_throw) {
    pf_set pf;

    pf.complete();

    auto f1 = pf[0].f.then_expect(te_generate_expected_value_throw);
    auto f2 = pf[1].f.then_expect(te_generate_expected_value_throw);
    auto f3 = pf[2].f.then_expect(te_generate_expected_value_throw);
    auto f4 = pf[3].f.then_expect(te_generate_expected_value_throw);

    ASSERT_THROW(f1.std_future().get(), std::runtime_error);
    ASSERT_THROW(f2.std_future().get(), std::runtime_error);
    ASSERT_THROW(f3.std_future().get(), std::runtime_error);
    ASSERT_THROW(f4.std_future().get(), std::runtime_error);
}

TEST(future_int, promote_tuple_to_variadic) {
    melon::promise<std::tuple<int, int>> p_t;
    auto f_t = p_t.get_future();

    melon::future<int, int> real_f = flatten(f_t);

    int a = 0;
    int b = 0;

    real_f.finally([&](melon::expected<int> ia, melon::expected<int> ib) {
        a = *ia;
        b = *ib;
    });

    ASSERT_EQ(0, a);
    ASSERT_EQ(0, b);

    p_t.set_value(std::make_tuple(2, 3));
    ASSERT_EQ(2, a);
    ASSERT_EQ(3, b);
}

TEST(future_int, random_timing) {
    std::random_device rd;
    std::mt19937 e2(rd());

    std::uniform_real_distribution<> dist(0, 0.002);

    for (int i = 0; i < 10000; ++i) {
        melon::promise<int> prom;
        melon::future<int> fut = prom.get_future();
        std::thread writer_thread([&]() {
            std::this_thread::sleep_for(
                    std::chrono::duration<double, std::milli>(dist(e2)));

            prom.set_value(12);
        });

        std::this_thread::sleep_for(
                std::chrono::duration<double, std::milli>(dist(e2)));

        ASSERT_EQ(12, fut.get());
        writer_thread.join();
    }
}