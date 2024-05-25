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




// Date: 2018/09/19 14:51:06

#include <pthread.h>
#include <gtest/gtest.h>
#include <gflags/gflags.h>
#include <melon/utility/macros.h>
#include <melon/fiber/fiber.h>
#include <melon/rpc/circuit_breaker.h>
#include <melon/rpc/socket.h>
#include <melon/rpc/server.h>
#include "echo.pb.h"

namespace {
void initialize_random() {
    srand(time(0));
}

const int kShortWindowSize = 500;
const int kLongWindowSize = 1000;
const int kShortWindowErrorPercent = 10;
const int kLongWindowErrorPercent = 5;
const int kMinIsolationDurationMs = 10;
const int kMaxIsolationDurationMs = 200;
const int kErrorCodeForFailed = 131;
const int kErrorCodeForSucc = 0;
const int kErrorCost = 1000;
const int kLatency = 1000;
const int kThreadNum = 3;
} // namespace

namespace melon {
DECLARE_int32(circuit_breaker_short_window_size);
DECLARE_int32(circuit_breaker_long_window_size);
DECLARE_int32(circuit_breaker_short_window_error_percent);
DECLARE_int32(circuit_breaker_long_window_error_percent);
DECLARE_int32(circuit_breaker_min_isolation_duration_ms);
DECLARE_int32(circuit_breaker_max_isolation_duration_ms);
} // namespace melon

int main(int argc, char* argv[]) {
    melon::FLAGS_circuit_breaker_short_window_size = kShortWindowSize;
    melon::FLAGS_circuit_breaker_long_window_size = kLongWindowSize;
    melon::FLAGS_circuit_breaker_short_window_error_percent = kShortWindowErrorPercent;
    melon::FLAGS_circuit_breaker_long_window_error_percent = kLongWindowErrorPercent;
    melon::FLAGS_circuit_breaker_min_isolation_duration_ms = kMinIsolationDurationMs;
    melon::FLAGS_circuit_breaker_max_isolation_duration_ms = kMaxIsolationDurationMs;
    testing::InitGoogleTest(&argc, argv);
    google::ParseCommandLineFlags(&argc, &argv, true);
    return RUN_ALL_TESTS();
}

pthread_once_t initialize_random_control = PTHREAD_ONCE_INIT;

struct FeedbackControl {
    FeedbackControl(int req_num, int error_percent,
                 melon::CircuitBreaker* circuit_breaker)
        : _req_num(req_num)
        , _error_percent(error_percent)
        , _circuit_breaker(circuit_breaker)
        , _healthy_cnt(0)
        , _unhealthy_cnt(0)
        , _healthy(true) {}
    int _req_num;
    int _error_percent;
    melon::CircuitBreaker* _circuit_breaker;
    int _healthy_cnt;
    int _unhealthy_cnt;
    bool _healthy;
};

class CircuitBreakerTest : public ::testing::Test {
protected:
    CircuitBreakerTest() {
        pthread_once(&initialize_random_control, initialize_random);
    };

    virtual ~CircuitBreakerTest() {};
    virtual void SetUp() {};
    virtual void TearDown() {};

    static void* feed_back_thread(void* data) {
        FeedbackControl* fc = static_cast<FeedbackControl*>(data);
        for (int i = 0; i < fc->_req_num; ++i) {
            bool healthy = false;
            if (rand() % 100 < fc->_error_percent) {
                healthy = fc->_circuit_breaker->OnCallEnd(kErrorCodeForFailed, kErrorCost);
            } else {
                healthy = fc->_circuit_breaker->OnCallEnd(kErrorCodeForSucc, kLatency);
            }
            fc->_healthy = healthy;
            if (healthy) {
                ++fc->_healthy_cnt;
            } else {
                ++fc->_unhealthy_cnt;
            }
        }
        return fc;
    }

    void StartFeedbackThread(std::vector<pthread_t>* thread_list,
                             std::vector<std::unique_ptr<FeedbackControl>>* fc_list,
                             int error_percent) {
        thread_list->clear();
        fc_list->clear();
        for (int i = 0; i < kThreadNum; ++i) {
            pthread_t tid = 0;
            FeedbackControl* fc =
                new FeedbackControl(2 * kLongWindowSize, error_percent, &_circuit_breaker);
            fc_list->emplace_back(fc);
            pthread_create(&tid, nullptr, feed_back_thread, fc);
            thread_list->push_back(tid);
        }
    }

    melon::CircuitBreaker _circuit_breaker;
};

TEST_F(CircuitBreakerTest, should_not_isolate) {
    std::vector<pthread_t> thread_list;
    std::vector<std::unique_ptr<FeedbackControl>> fc_list;
    StartFeedbackThread(&thread_list, &fc_list, 3);
    for (int  i = 0; i < kThreadNum; ++i) {
        void* ret_data = nullptr;
        ASSERT_EQ(pthread_join(thread_list[i], &ret_data), 0);
        FeedbackControl* fc = static_cast<FeedbackControl*>(ret_data);
        EXPECT_EQ(fc->_unhealthy_cnt, 0);
        EXPECT_TRUE(fc->_healthy);
    }
}

TEST_F(CircuitBreakerTest, should_isolate) {
    std::vector<pthread_t> thread_list;
    std::vector<std::unique_ptr<FeedbackControl>> fc_list;
    StartFeedbackThread(&thread_list, &fc_list, 50);
    for (int  i = 0; i < kThreadNum; ++i) {
        void* ret_data = nullptr;
        ASSERT_EQ(pthread_join(thread_list[i], &ret_data), 0);
        FeedbackControl* fc = static_cast<FeedbackControl*>(ret_data);
        EXPECT_GT(fc->_unhealthy_cnt, 0);
        EXPECT_FALSE(fc->_healthy);
    }
}

TEST_F(CircuitBreakerTest, isolation_duration_grow_and_reset) {
    std::vector<pthread_t> thread_list;
    std::vector<std::unique_ptr<FeedbackControl>> fc_list;
    StartFeedbackThread(&thread_list, &fc_list, 100);
    for (int  i = 0; i < kThreadNum; ++i) {
        void* ret_data = nullptr;
        ASSERT_EQ(pthread_join(thread_list[i], &ret_data), 0);
        FeedbackControl* fc = static_cast<FeedbackControl*>(ret_data);
        EXPECT_FALSE(fc->_healthy);
        EXPECT_LE(fc->_healthy_cnt, kShortWindowSize);
        EXPECT_GT(fc->_unhealthy_cnt, 0);
    }
    EXPECT_EQ(_circuit_breaker.isolation_duration_ms(), kMinIsolationDurationMs);

    _circuit_breaker.Reset();
    StartFeedbackThread(&thread_list, &fc_list, 100);
    for (int  i = 0; i < kThreadNum; ++i) {
        void* ret_data = nullptr;
        ASSERT_EQ(pthread_join(thread_list[i], &ret_data), 0);
        FeedbackControl* fc = static_cast<FeedbackControl*>(ret_data);
        EXPECT_FALSE(fc->_healthy);
        EXPECT_LE(fc->_healthy_cnt, kShortWindowSize);
        EXPECT_GT(fc->_unhealthy_cnt, 0);
    }
    EXPECT_EQ(_circuit_breaker.isolation_duration_ms(), kMinIsolationDurationMs * 2);

    _circuit_breaker.Reset();
    StartFeedbackThread(&thread_list, &fc_list, 100);
    for (int  i = 0; i < kThreadNum; ++i) {
        void* ret_data = nullptr;
        ASSERT_EQ(pthread_join(thread_list[i], &ret_data), 0);
        FeedbackControl* fc = static_cast<FeedbackControl*>(ret_data);
        EXPECT_FALSE(fc->_healthy);
        EXPECT_LE(fc->_healthy_cnt, kShortWindowSize);
        EXPECT_GT(fc->_unhealthy_cnt, 0);
    }
    EXPECT_EQ(_circuit_breaker.isolation_duration_ms(), kMinIsolationDurationMs * 4);

    _circuit_breaker.Reset();
    ::usleep((kMaxIsolationDurationMs + kMinIsolationDurationMs) * 1000);
    StartFeedbackThread(&thread_list, &fc_list, 100);
    for (int  i = 0; i < kThreadNum; ++i) {
        void* ret_data = nullptr;
        ASSERT_EQ(pthread_join(thread_list[i], &ret_data), 0);
        FeedbackControl* fc = static_cast<FeedbackControl*>(ret_data);
        EXPECT_FALSE(fc->_healthy);
        EXPECT_LE(fc->_healthy_cnt, kShortWindowSize);
        EXPECT_GT(fc->_unhealthy_cnt, 0);
    }
    EXPECT_EQ(_circuit_breaker.isolation_duration_ms(), kMinIsolationDurationMs);

}

TEST_F(CircuitBreakerTest, maximum_isolation_duration) {
    melon::FLAGS_circuit_breaker_max_isolation_duration_ms =
        melon::FLAGS_circuit_breaker_min_isolation_duration_ms + 1;
    ASSERT_LT(melon::FLAGS_circuit_breaker_max_isolation_duration_ms,
              2 * melon::FLAGS_circuit_breaker_min_isolation_duration_ms);
    std::vector<pthread_t> thread_list;
    std::vector<std::unique_ptr<FeedbackControl>> fc_list;

    _circuit_breaker.Reset();
    StartFeedbackThread(&thread_list, &fc_list, 100);
    for (int  i = 0; i < kThreadNum; ++i) {
        void* ret_data = nullptr;
        ASSERT_EQ(pthread_join(thread_list[i], &ret_data), 0);
        FeedbackControl* fc = static_cast<FeedbackControl*>(ret_data);
        EXPECT_FALSE(fc->_healthy);
        EXPECT_LE(fc->_healthy_cnt, kShortWindowSize);
        EXPECT_GT(fc->_unhealthy_cnt, 0);
    }
    EXPECT_EQ(_circuit_breaker.isolation_duration_ms(),
              melon::FLAGS_circuit_breaker_max_isolation_duration_ms);
}
