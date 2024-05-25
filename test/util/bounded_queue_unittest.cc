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


// Date: Sun Nov  3 19:16:50 PST 2019

#include <iostream>
#include <melon/utility/containers/bounded_queue.h>
#include <gtest/gtest.h>

namespace {

TEST(BoundedQueueTest, sanity) {
    const int N = 36;
    char storage[N * sizeof(int)];
    mutil::BoundedQueue<int> q(storage, sizeof(storage), mutil::NOT_OWN_STORAGE);
    ASSERT_EQ(0ul, q.size());
    ASSERT_TRUE(q.empty());
    ASSERT_TRUE(NULL == q.top());
    ASSERT_TRUE(NULL == q.bottom());
    for (int i = 1; i <= N; ++i) {
        if (i % 2 == 0) {
            ASSERT_TRUE(q.push(i));
        } else {
            int* p = q.push();
            ASSERT_TRUE(p);
            *p = i;
        }
        ASSERT_EQ((size_t)i, q.size());
        ASSERT_EQ(1, *q.top());
        ASSERT_EQ(i, *q.bottom());
    }
    ASSERT_FALSE(q.push(N+1));
    ASSERT_FALSE(q.push_top(N+1));
    ASSERT_EQ((size_t)N, q.size());
    ASSERT_FALSE(q.empty());
    ASSERT_TRUE(q.full());

    for (int i = 1; i <= N; ++i) {
        ASSERT_EQ(i, *q.top());
        ASSERT_EQ(N, *q.bottom());
        if (i % 2 == 0) {
            int tmp = 0;
            ASSERT_TRUE(q.pop(&tmp));
            ASSERT_EQ(tmp, i);
        } else {
            ASSERT_TRUE(q.pop());
        }
        ASSERT_EQ((size_t)(N-i), q.size());
    }
    ASSERT_EQ(0ul, q.size());
    ASSERT_TRUE(q.empty());
    ASSERT_FALSE(q.full());
    ASSERT_FALSE(q.pop());

    for (int i = 1; i <= N; ++i) {
        if (i % 2 == 0) {
            ASSERT_TRUE(q.push_top(i));
        } else {
            int* p = q.push_top();
            ASSERT_TRUE(p);
            *p = i;
        }
        ASSERT_EQ((size_t)i, q.size());
        ASSERT_EQ(i, *q.top());
        ASSERT_EQ(1, *q.bottom());
    }
    ASSERT_FALSE(q.push(N+1));
    ASSERT_FALSE(q.push_top(N+1));
    ASSERT_EQ((size_t)N, q.size());
    ASSERT_FALSE(q.empty());
    ASSERT_TRUE(q.full());

    for (int i = 1; i <= N; ++i) {
        ASSERT_EQ(N, *q.top());
        ASSERT_EQ(i, *q.bottom());
        if (i % 2 == 0) {
            int tmp = 0;
            ASSERT_TRUE(q.pop_bottom(&tmp));
            ASSERT_EQ(tmp, i);
        } else {
            ASSERT_TRUE(q.pop_bottom());
        }
        ASSERT_EQ((size_t)(N-i), q.size());
    }
    ASSERT_EQ(0ul, q.size());
    ASSERT_TRUE(q.empty());
    ASSERT_FALSE(q.full());
    ASSERT_FALSE(q.pop());
}

} // anonymous namespace
