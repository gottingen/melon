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
#include <melon/utility/time.h>
#include <melon/utility/macros.h>
#include <turbo/log/logging.h>
#include <melon/fiber/task_group.h>
#include <melon/fiber/fiber.h>

namespace {
void* sleeper(void* arg) {
    fiber_usleep((long)arg);
    return NULL;
}

TEST(ListTest, join_thread_by_list) {
    fiber_list_t list;
    ASSERT_EQ(0, fiber_list_init(&list, 0, 0));
    std::vector<fiber_t> tids;
    for (size_t i = 0; i < 10; ++i) {
        fiber_t th;
        ASSERT_EQ(0, fiber_start_urgent(
                      &th, NULL, sleeper, (void*)10000/*10ms*/));
        ASSERT_EQ(0, fiber_list_add(&list, th));
        tids.push_back(th);
    }
    ASSERT_EQ(0, fiber_list_join(&list));
    for (size_t i = 0; i < tids.size(); ++i) {
        ASSERT_FALSE(fiber::TaskGroup::exists(tids[i]));
    }
    fiber_list_destroy(&list);
}

TEST(ListTest, join_a_destroyed_list) {
    fiber_list_t list;
    ASSERT_EQ(0, fiber_list_init(&list, 0, 0));
    fiber_t th;
    ASSERT_EQ(0, fiber_start_urgent(
                  &th, NULL, sleeper, (void*)10000/*10ms*/));
    ASSERT_EQ(0, fiber_list_add(&list, th));
    ASSERT_EQ(0, fiber_list_join(&list));
    fiber_list_destroy(&list);
    ASSERT_EQ(EINVAL, fiber_list_join(&list));
}
} // namespace
