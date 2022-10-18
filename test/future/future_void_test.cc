
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/

#include <iostream>
#include <queue>
#include "testing/gtest_wrap.h"
#include "melon/future/future.h"
#include <array>
#include <queue>


namespace {

    struct prom_fut {
        prom_fut() { f = p.get_future(); }

        void get() { f.std_future().get(); }

        melon::promise<void> p;
        melon::future<void> f;
    };

    struct pf_set {
        prom_fut pf[4];

        prom_fut &operator[](std::size_t i) { return pf[i]; }

        void complete() {
            pf[0].p.set_value();
            pf[1].p.set_exception(std::make_exception_ptr(std::logic_error("nope")));
            pf[2].p.finish(melon::expected<void>());
            pf[3].p.finish(
                    melon::unexpected(std::make_exception_ptr(std::logic_error(""))));
        }
    };

    int expect_noop_count = 0;

    void expected_noop(melon::expected<void>) { ++expect_noop_count; }

    void expected_noop_fail(melon::expected<void>) { throw std::runtime_error("dead"); }

    void no_op() {}

    void failure() { throw std::runtime_error("dead"); }

    int return_int() { return 1; }

    int return_int_fail() { throw std::runtime_error(""); }


    melon::expected<void> expected_cb() { return {}; }

    melon::expected<void> expected_cb_fail() {
        return melon::unexpected{std::make_exception_ptr(std::runtime_error("yikes"))};
    }
}  // namespace

TEST(future_void, fundamental_expectations) {
    // These tests failing do not mean that the library doesn't work.
    // It's just that some architecture-related assumptions made are not being
    // met, so performance might be sub-optimal.

    // The special immediate queue type qualifies as having static push.
    ASSERT_TRUE(melon::future_internal::has_static_push_v<melon::future_internal::immediate_queue>);

    // Base handler should be nothing but a vtable pointer.
    ASSERT_EQ(sizeof(void *),
              sizeof(melon::future_internal::future_handler_base<melon::future_internal::immediate_queue, void>));
}

TEST(future_void, blank) { melon::future<void> fut; }

TEST(future_void, unfilled_promise_failiure) {
    melon::future<void> fut;
    {
        melon::promise<void> p;
        fut = p.get_future();
    }

    ASSERT_THROW(fut.std_future().get(), melon::unfull_filled_promise);
}

TEST(future_void, preloaded_std_get) {
    pf_set pf;

    pf.complete();

    ASSERT_NO_THROW(pf[0].get());
    ASSERT_THROW(pf[1].get(), std::logic_error);
    ASSERT_NO_THROW(pf[2].get());
    ASSERT_THROW(pf[3].get(), std::logic_error);
}

TEST(future_void, delayed_std_get) {
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

    ASSERT_NO_THROW(std_f1.get());
    ASSERT_THROW(std_f2.get(), std::logic_error);
    ASSERT_NO_THROW(std_f3.get());
    ASSERT_THROW(std_f4.get(), std::logic_error);

    thread.join();
}

TEST(future_void, then_noop_pre) {
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

TEST(future_void, then_noop_post) {
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

TEST(future_void, then_failure_pre) {
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

TEST(future_void, then_failure_post) {
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

TEST(future_void, then_expect_success_pre) {
    pf_set pf;

    auto f1 = pf[0].f.then_expect(expected_noop);
    auto f2 = pf[1].f.then_expect(expected_noop);
    auto f3 = pf[2].f.then_expect(expected_noop);
    auto f4 = pf[3].f.then_expect(expected_noop);

    pf.complete();

    ASSERT_NO_THROW(f1.std_future().get());
    ASSERT_NO_THROW(f2.std_future().get());
    ASSERT_NO_THROW(f3.std_future().get());
    ASSERT_NO_THROW(f4.std_future().get());
}

TEST(future_void, then_expect_success_post) {
    pf_set pf;

    auto f1 = pf[0].f.then_expect(expected_noop);
    auto f2 = pf[1].f.then_expect(expected_noop);
    auto f3 = pf[2].f.then_expect(expected_noop);
    auto f4 = pf[3].f.then_expect(expected_noop);

    pf.complete();

    ASSERT_NO_THROW(f1.std_future().get());
    ASSERT_NO_THROW(f2.std_future().get());
    ASSERT_NO_THROW(f3.std_future().get());
    ASSERT_NO_THROW(f4.std_future().get());
}

TEST(future_void, then_expect_failure_pre) {
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

TEST(future_void, then_expect_failure_post) {
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

TEST(future_void, then_expect_finally_success_pre) {
    pf_set pf;

    auto ref = expect_noop_count;

    pf[0].f.finally(expected_noop);
    pf[1].f.finally(expected_noop);
    pf[2].f.finally(expected_noop);
    pf[3].f.finally(expected_noop);

    pf.complete();

    ASSERT_EQ(4 + ref, expect_noop_count);
}

TEST(future_void, then_expect_finally_success_post) {
    pf_set pf;

    auto ref = expect_noop_count;

    pf.complete();

    pf[0].f.finally(expected_noop);
    pf[1].f.finally(expected_noop);
    pf[2].f.finally(expected_noop);
    pf[3].f.finally(expected_noop);

    ASSERT_EQ(4 + ref, expect_noop_count);
}

TEST(future_void, chain_to_int) {
    pf_set pf;

    pf.complete();

    auto f1 = pf[0].f.then(return_int);
    auto f2 = pf[1].f.then(return_int);
    auto f3 = pf[2].f.then(return_int);
    auto f4 = pf[3].f.then(return_int);

    ASSERT_EQ(1, f1.std_future().get());
    ASSERT_THROW(f2.std_future().get(), std::logic_error);
    ASSERT_EQ(1, f3.std_future().get());
    ASSERT_THROW(f4.std_future().get(), std::logic_error);
}

TEST(future_void, chain_to_int_fail) {
    pf_set pf;

    pf.complete();

    auto f1 = pf[0].f.then(return_int_fail);
    auto f2 = pf[1].f.then(return_int_fail);
    auto f3 = pf[2].f.then(return_int_fail);
    auto f4 = pf[3].f.then(return_int_fail);

    ASSERT_THROW(f1.std_future().get(), std::runtime_error);
    ASSERT_THROW(f2.std_future().get(), std::logic_error);
    ASSERT_THROW(f3.std_future().get(), std::runtime_error);
    ASSERT_THROW(f4.std_future().get(), std::logic_error);
}

TEST(future_void, expected_returning_callback) {
    pf_set pf;

    pf.complete();

    auto f1 = pf[0].f.then(expected_cb);
    auto f2 = pf[1].f.then(expected_cb);
    auto f3 = pf[2].f.then(expected_cb);
    auto f4 = pf[3].f.then(expected_cb);

    ASSERT_NO_THROW(f1.std_future().get());
    ASSERT_THROW(f2.std_future().get(), std::logic_error);
    ASSERT_NO_THROW(f3.std_future().get());
    ASSERT_THROW(f4.std_future().get(), std::logic_error);
}

TEST(future_void, expected_returning_callback_fail) {
    pf_set pf;

    pf.complete();

    auto f1 = pf[0].f.then(expected_cb_fail);
    auto f2 = pf[1].f.then(expected_cb_fail);
    auto f3 = pf[2].f.then(expected_cb_fail);
    auto f4 = pf[3].f.then(expected_cb_fail);

    ASSERT_THROW(f1.std_future().get(), std::runtime_error);
    ASSERT_THROW(f2.std_future().get(), std::logic_error);
    ASSERT_THROW(f3.std_future().get(), std::runtime_error);
    ASSERT_THROW(f4.std_future().get(), std::logic_error);
}