//
// Copyright (C) 2024 EA group inc.
// Author: Jeff.li lijippy@163.com
// All rights reserved.
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as published
// by the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.
//
//


// Date: 2016/06/03 13:25:44

#include <gtest/gtest.h>
#include <melon/fiber/countdown_event.h>
#include <melon/utility/atomicops.h>
#include <melon/utility/time.h>

namespace {
struct Arg {
    fiber::CountdownEvent event;
    std::atomic<int> num_sig;
};

void *signaler(void *arg) {
    Arg* a = (Arg*)arg;
    a->num_sig.fetch_sub(1, std::memory_order_relaxed);
    a->event.signal();
    return NULL;
}

TEST(CountdonwEventTest, sanity) {
    for (int n = 1; n < 10; ++n) {
        Arg a;
        a.num_sig = n;
        a.event.reset(n);
        for (int i = 0; i < n; ++i) {
            fiber_t tid;
            ASSERT_EQ(0, fiber_start_urgent(&tid, NULL, signaler, &a));
        }
        a.event.wait();
        ASSERT_EQ(0, a.num_sig.load(std::memory_order_relaxed));
    }
}

TEST(CountdonwEventTest, timed_wait) {
    fiber::CountdownEvent event;
    int rc = event.timed_wait(mutil::milliseconds_from_now(100));
    ASSERT_EQ(rc, ETIMEDOUT);
    event.signal();
    rc = event.timed_wait(mutil::milliseconds_from_now(100));
    ASSERT_EQ(rc, 0);
    fiber::CountdownEvent event1;
    event1.signal();
    rc = event.timed_wait(mutil::milliseconds_from_now(1));
    ASSERT_EQ(rc, 0);
}
} // namespace
