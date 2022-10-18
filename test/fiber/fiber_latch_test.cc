
#include "testing/gtest_wrap.h"
#include <melon/fiber/fiber_latch.h>
#include "melon/base/static_atomic.h"
#include "melon/times/time.h"

namespace {
    struct Arg {
        melon::fiber_latch latcher;
        std::atomic<int> num_sig;
    };

    void *signaler(void *arg) {
        Arg *a = (Arg *) arg;
        a->num_sig.fetch_sub(1, std::memory_order_relaxed);
        a->latcher.signal();
        return nullptr;
    }

    TEST(FiberLatchTest, sanity) {
        for (int n = 1; n < 10; ++n) {
            Arg a;
            a.num_sig = n;
            a.latcher.reset(n);
            for (int i = 0; i < n; ++i) {
                fiber_id_t tid;
                ASSERT_EQ(0, fiber_start_urgent(&tid, nullptr, signaler, &a));
            }
            a.latcher.wait();
            ASSERT_EQ(0, a.num_sig.load(std::memory_order_relaxed));
        }
    }

    TEST(FiberLatchTest, timed_wait) {
        melon::fiber_latch latcher;
        auto ts = melon::time_point::future_unix_millis(100).to_timespec();
        int rc = latcher.timed_wait(&ts);
        ASSERT_EQ(rc, ETIMEDOUT);
        latcher.signal();
        auto ts1 = melon::time_point::future_unix_millis(100).to_timespec();
        rc = latcher.timed_wait(&ts1);
        ASSERT_EQ(rc, 0);
        melon::fiber_latch latcher1;
        latcher1.signal();
        auto ts2 = melon::time_point::future_unix_millis(1).to_timespec();
        rc = latcher.timed_wait(&ts2);
        ASSERT_EQ(rc, 0);
    }
} // namespace
