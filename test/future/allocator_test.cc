
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
    template<typename T>
    struct test_alloc {
        using value_type = T;

        template<typename U>
        struct rebind {
            using other = test_alloc<U>;
        };

        test_alloc(std::atomic<int> *counter, std::atomic<int> *total)
                : counter_(counter), total_(total) {}

        T *allocate(std::size_t count) {
            ++*counter_;
            ++*total_;
            return reinterpret_cast<T *>(std::malloc(count * sizeof(T)));
        }

        void deallocate(T *ptr, std::size_t) {
            std::free(ptr);
            --*counter_;
        }

        template<typename U>
        explicit test_alloc(const test_alloc<U> &rhs) {
            counter_ = rhs.counter_;
            total_ = rhs.total_;
        }

        template<typename U>
        test_alloc &operator=(test_alloc<U> &rhs) {
            counter_ = rhs.counter_;
            total_ = rhs.total_;
            return *this;
        }

        std::atomic<int> *counter_;
        std::atomic<int> *total_;
    };

    template<>
    struct test_alloc<void> {
        using value_type = void;

        template<typename U>
        struct rebind {
            using other = test_alloc<U>;
        };

        test_alloc(std::atomic<int> *counter, std::atomic<int> *total)
                : counter_(counter), total_(total) {}

        template<typename U>
        explicit test_alloc(const test_alloc<U> &rhs) {
            counter_ = rhs.counter_;
            total_ = rhs.total_;
        }

        template<typename U>
        test_alloc &operator=(test_alloc<U> &rhs) {
            counter_ = rhs.counter_;
            total_ = rhs.total_;
            return *this;
        }

        std::atomic<int> *counter_;
        std::atomic<int> *total_;
    };

    using future_type = melon::basic_future<test_alloc<void>, int>;
    using promise_type = melon::basic_promise<test_alloc<void>, int>;

    struct prom_fut {
        prom_fut(std::atomic<int> *counter, std::atomic<int> *total)
                : p(test_alloc<void>(counter, total)) {
            f = p.get_future();
        }

        int get() { return f.std_future().get(); }

        future_type f;
        promise_type p;
    };

    struct pf_set {
        pf_set()
                : pf{prom_fut(&counter, &total), prom_fut(&counter, &total),
                     prom_fut(&counter, &total), prom_fut(&counter, &total)} {}

        ~pf_set() {
            EXPECT_TRUE(counter == 0);
        }

        std::atomic<int> counter = 0;
        std::atomic<int> total = 0;
        std::array<prom_fut, 4> pf;

        prom_fut &operator[](std::size_t i) { return pf[i]; }

        void complete() {
            pf[0].p.set_value(1);
            pf[1].p.set_exception(std::make_exception_ptr(std::logic_error("nope")));
            pf[2].p.finish(melon::expected<int, std::exception_ptr>(1));
            pf[3].p.finish(
                    melon::unexpected(std::make_exception_ptr(std::logic_error(""))));
        }
    };

    void no_op(int i) { ASSERT_TRUE(i == 1); }

    void failure(int i) {
        ASSERT_TRUE(i == 1);
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



TEST(future, blank) {
    future_type fut;
}

TEST(future, unfullfilled_fails) {
    std::atomic<int> counter = 0;
    std::atomic<int> total = 0;

    future_type fut;
    {
        promise_type p(test_alloc<void>(&counter, &
        total));
        fut = p.get_future();
    }

    ASSERT_THROW(fut.std_future().get(), melon::unfull_filled_promise);
}

TEST(future, preloaded_std_get) {
    pf_set pf;

    pf.complete();

    ASSERT_EQ(1, pf[0].get());
    ASSERT_THROW(pf[1].get(), std::logic_error);
    ASSERT_EQ(1, pf[2].get());
    ASSERT_THROW(pf[3].get(), std::logic_error);
}

TEST(future, delayed_std_get) {
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

TEST(future, then_noop_pre) {
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

TEST(future, then_noop_post) {
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

TEST(future, then_noop_pre_large_callback) {
    pf_set pf;

    struct payload_t {
        int x = 0;
        int y = 0;
        int z = 0;
    };

    payload_t payload;

    payload.x = 1;
    payload.y = 1;
    payload.z = 1;

    auto callback = [payload](int) {};

    auto f1 = pf[0].f.then(callback);
    auto f2 = pf[1].f.then(callback);
    auto f3 = pf[2].f.then(callback);
    auto f4 = pf[3].f.then(callback);

    pf.complete();

    ASSERT_NO_THROW(f1.std_future().get());
    ASSERT_THROW(f2.std_future().get(), std::logic_error);
    ASSERT_NO_THROW(f3.std_future().get());
    ASSERT_THROW(f4.std_future().get(), std::logic_error);
}

TEST(future, then_failure_pre) {
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

TEST(future, then_failure_post) {
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

TEST(future, then_expect_success_pre) {
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

TEST(future, then_expect_success_post) {
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

TEST(future, then_expect_failure_pre) {
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

TEST(future, then_expect_failure_post) {
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

TEST(future, then_expect_finally_success_pre) {
    pf_set pf;

    auto ref = expect_noop_count;

    pf[0].f.finally(expected_noop);
    pf[1].f.finally(expected_noop);
    pf[2].f.finally(expected_noop);
    pf[3].f.finally(expected_noop);

    pf.complete();

    ASSERT_EQ(4 + ref, expect_noop_count);
}

TEST(future, then_expect_finally_success_post) {
    pf_set pf;

    auto ref = expect_noop_count;

    pf.complete();

    pf[0].f.finally(expected_noop);
    pf[1].f.finally(expected_noop);
    pf[2].f.finally(expected_noop);
    pf[3].f.finally(expected_noop);

    ASSERT_EQ(4 + ref, expect_noop_count);
}


TEST(future, expected_returning_callback) {
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

TEST(future, expected_returning_callback_fail) {
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

TEST(future, expected_returning_callback_throw) {
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

TEST(future, te_expected_returning_callback) {
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

TEST(future, te_expected_returning_callback_fail) {
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

TEST(future, te_expected_returning_callback_throw) {
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
