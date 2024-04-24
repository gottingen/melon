// Copyright 2023 The Elastic-AI Authors.
// part of Elastic AI Search
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//


#include <gtest/gtest.h>
#include "melon/utility/time.h"
#include "melon/utility/macros.h"
#include "melon/utility/logging.h"
#include "melon/fiber/task_group.h"
#include "melon/fiber/fiber.h"

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
