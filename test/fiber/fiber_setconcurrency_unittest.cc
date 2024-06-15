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


#include <gtest/gtest.h>
#include <gflags/gflags.h>
#include <melon/utility/atomicops.h>
#include <melon/utility/time.h>
#include <melon/utility/macros.h>
#include <turbo/log/logging.h>
#include <melon/base/thread_local.h>
#include <melon/fiber/butex.h>
#include <turbo/log/logging.h>
#include <melon/fiber/fiber.h>
#include <melon/fiber/task_control.h>

namespace fiber {
    extern TaskControl* g_task_control;
}

namespace {
void* dummy(void*) {
    return NULL;
}

TEST(FiberTest, setconcurrency) {
    ASSERT_EQ(8 + FIBER_EPOLL_THREAD_NUM, (size_t)fiber_getconcurrency());
    ASSERT_EQ(EINVAL, fiber_setconcurrency(FIBER_MIN_CONCURRENCY - 1));
    ASSERT_EQ(EINVAL, fiber_setconcurrency(0));
    ASSERT_EQ(EINVAL, fiber_setconcurrency(-1));
    ASSERT_EQ(EINVAL, fiber_setconcurrency(FIBER_MAX_CONCURRENCY + 1));
    ASSERT_EQ(0, fiber_setconcurrency(FIBER_MIN_CONCURRENCY));
    ASSERT_EQ(FIBER_MIN_CONCURRENCY, fiber_getconcurrency());
    ASSERT_EQ(0, fiber_setconcurrency(FIBER_MIN_CONCURRENCY + 1));
    ASSERT_EQ(FIBER_MIN_CONCURRENCY + 1, fiber_getconcurrency());
    ASSERT_EQ(0, fiber_setconcurrency(FIBER_MIN_CONCURRENCY));  // smaller value
    fiber_t th;
    ASSERT_EQ(0, fiber_start_urgent(&th, NULL, dummy, NULL));
    ASSERT_EQ(FIBER_MIN_CONCURRENCY + 1, fiber_getconcurrency());
    ASSERT_EQ(0, fiber_setconcurrency(FIBER_MIN_CONCURRENCY + 5));
    ASSERT_EQ(FIBER_MIN_CONCURRENCY + 5, fiber_getconcurrency());
    ASSERT_EQ(EPERM, fiber_setconcurrency(FIBER_MIN_CONCURRENCY + 1));
    ASSERT_EQ(FIBER_MIN_CONCURRENCY + 5, fiber_getconcurrency());
}

static mutil::atomic<int> *odd;
static mutil::atomic<int> *even;

static mutil::atomic<int> nfibers(0);
static mutil::atomic<int> npthreads(0);
static MELON_THREAD_LOCAL bool counted = false;
static mutil::atomic<bool> stop (false);

static void *odd_thread(void *) {
    nfibers.fetch_add(1);
    while (!stop) {
        if (!counted) {
            counted = true;
            npthreads.fetch_add(1);
        }
        fiber::butex_wake_all(even);
        fiber::butex_wait(odd, 0, NULL);
    }
    return NULL;
}

static void *even_thread(void *) {
    nfibers.fetch_add(1);
    while (!stop) {
        if (!counted) {
            counted = true;
            npthreads.fetch_add(1);
        }
        fiber::butex_wake_all(odd);
        fiber::butex_wait(even, 0, NULL);
    }
    return NULL;
}

TEST(FiberTest, setconcurrency_with_running_fiber) {
    odd = fiber::butex_create_checked<mutil::atomic<int> >();
    even = fiber::butex_create_checked<mutil::atomic<int> >();
    ASSERT_TRUE(odd != NULL && even != NULL);
    *odd = 0;
    *even = 0;
    std::vector<fiber_t> tids;
    const int N = 500;
    for (int i = 0; i < N; ++i) {
        fiber_t tid;
        fiber_start_background(&tid, &FIBER_ATTR_SMALL, odd_thread, NULL);
        tids.push_back(tid);
        fiber_start_background(&tid, &FIBER_ATTR_SMALL, even_thread, NULL);
        tids.push_back(tid);
    }
    for (int i = 100; i <= N; ++i) {
        ASSERT_EQ(0, fiber_setconcurrency(i));
        ASSERT_EQ(i, fiber_getconcurrency());
    }
    usleep(1000 * N);
    *odd = 1;
    *even = 1;
    stop =  true;
    fiber::butex_wake_all(odd);
    fiber::butex_wake_all(even);
    for (size_t i = 0; i < tids.size(); ++i) {
        fiber_join(tids[i], NULL);
    }
    LOG(INFO) << "All fibers has quit";
    ASSERT_EQ(2*N, nfibers);
    // This is not necessarily true, not all workers need to run sth.
    //ASSERT_EQ(N, npthreads);
    LOG(INFO) << "Touched pthreads=" << npthreads;
}

void* sleep_proc(void*) {
    usleep(100000);
    return NULL;
}

void* add_concurrency_proc(void*) {
    fiber_t tid;
    fiber_start_background(&tid, &FIBER_ATTR_SMALL, sleep_proc, NULL);
    fiber_join(tid, NULL);
    return NULL;
}

bool set_min_concurrency(int num) {
    std::stringstream ss;
    ss << num;
    std::string ret = google::SetCommandLineOption("fiber_min_concurrency", ss.str().c_str());
    return !ret.empty();
}

int get_min_concurrency() {
    std::string ret;
    google::GetCommandLineOption("fiber_min_concurrency", &ret);
    return atoi(ret.c_str());
}

TEST(FiberTest, min_concurrency) {
    ASSERT_EQ(1, set_min_concurrency(-1)); // set min success
    ASSERT_EQ(1, set_min_concurrency(0)); // set min success
    ASSERT_EQ(0, get_min_concurrency());
    int conn = fiber_getconcurrency();
    int add_conn = 100;

    ASSERT_EQ(0, set_min_concurrency(conn + 1)); // set min failed
    ASSERT_EQ(0, get_min_concurrency());

    ASSERT_EQ(1, set_min_concurrency(conn - 1)); // set min success
    ASSERT_EQ(conn - 1, get_min_concurrency());

    ASSERT_EQ(EINVAL, fiber_setconcurrency(conn - 2)); // set max failed
    ASSERT_EQ(0, fiber_setconcurrency(conn + add_conn + 1)); // set max success
    ASSERT_EQ(0, fiber_setconcurrency(conn + add_conn)); // set max success
    ASSERT_EQ(conn + add_conn, fiber_getconcurrency());
    ASSERT_EQ(conn, fiber::g_task_control->concurrency());

    ASSERT_EQ(1, set_min_concurrency(conn + 1)); // set min success
    ASSERT_EQ(conn + 1, get_min_concurrency());
    ASSERT_EQ(conn + 1, fiber::g_task_control->concurrency());

    std::vector<fiber_t> tids;
    for (int i = 0; i < conn; ++i) {
        fiber_t tid;
        fiber_start_background(&tid, &FIBER_ATTR_SMALL, sleep_proc, NULL);
        tids.push_back(tid);
    }
    for (int i = 0; i < add_conn; ++i) {
        fiber_t tid;
        fiber_start_background(&tid, &FIBER_ATTR_SMALL, add_concurrency_proc, NULL);
        tids.push_back(tid);
    }
    for (size_t i = 0; i < tids.size(); ++i) {
        fiber_join(tids[i], NULL);
    }
    ASSERT_EQ(conn + add_conn, fiber_getconcurrency());
    ASSERT_EQ(conn + add_conn, fiber::g_task_control->concurrency());
}

int current_tag(int tag) {
    std::stringstream ss;
    ss << tag;
    std::string ret = google::SetCommandLineOption("fiber_current_tag", ss.str().c_str());
    return !(ret.empty());
}

TEST(FiberTest, current_tag) {
    ASSERT_EQ(false, current_tag(-1));
    ASSERT_EQ(true, current_tag(0));
    ASSERT_EQ(false, current_tag(1));
}

int concurrency_by_tag(int num) {
    std::stringstream ss;
    ss << num;
    std::string ret =
        google::SetCommandLineOption("fiber_concurrency_by_tag", ss.str().c_str());
    return !(ret.empty());
}

TEST(FiberTest, concurrency_by_tag) {
    ASSERT_EQ(concurrency_by_tag(1), true);
    ASSERT_EQ(concurrency_by_tag(1), false);
    auto con = fiber_getconcurrency_by_tag(0);
    ASSERT_EQ(concurrency_by_tag(con), true);
    ASSERT_EQ(concurrency_by_tag(con + 1), false);
    fiber_setconcurrency(con + 1);
    ASSERT_EQ(concurrency_by_tag(con + 1), true);
}

} // namespace
