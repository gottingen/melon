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

#pragma once

#include <turbo/flags/declare.h>



// Number of pthread workers default 8 + FIBER_EPOLL_THREAD_NUM(1)
TURBO_DECLARE_FLAG(int32_t, fiber_concurrency);

// Initial number of pthread workers workers will be
// added on-demand. The laziness is disabled when this value is non-positive,
// and workers will be created eagerly according to -fiber_concurrency and fiber_setconcurrency()
// default 0
TURBO_DECLARE_FLAG(int32_t, fiber_min_concurrency);

// set fiber concurrency for this tag default 0
TURBO_DECLARE_FLAG(int32_t, fiber_current_tag);

// Number of pthread workers of FLAGS_fiber_current_tag default 0
TURBO_DECLARE_FLAG(int32_t, fiber_concurrency_by_tag);

// size of small stacks default 32768
TURBO_DECLARE_FLAG(int32_t, stack_size_small);

// size of normal stacks default 1048576
TURBO_DECLARE_FLAG(int32_t, stack_size_normal);

// size of large stacks default 8388608
TURBO_DECLARE_FLAG(int32_t, stack_size_large);

// size of guard page, allocate stacks by malloc if it's 0(not recommended) default 4096
TURBO_DECLARE_FLAG(int32_t, guard_page_size);

// maximum small stacks cached by each thread default 32
TURBO_DECLARE_FLAG(int32_t, tc_stack_small);

// maximum normal stacks cached by each thread default 8
TURBO_DECLARE_FLAG(int32_t, tc_stack_normal);

// delay deletion of TaskGroup for so many seconds default 1
TURBO_DECLARE_FLAG(int32_t, task_group_delete_delay);

// capacity of runqueue in each TaskGroup default 4096
TURBO_DECLARE_FLAG(int32_t, task_group_runqueue_capacity);

// TaskGroup yields so many times before idle default 0
TURBO_DECLARE_FLAG(int32_t, task_group_yield_before_idle);

// TaskGroup will be grouped by number ntags default 1
TURBO_DECLARE_FLAG(int32_t, task_group_ntags);

// When this flags is on, The time from fiber creation to first run will be recorded and shown in /vars default false
TURBO_DECLARE_FLAG(bool, show_fiber_creation_in_vars);

// Show per-worker usage in /vars/fiber_per_worker_usage_<tid> default false
TURBO_DECLARE_FLAG(bool, show_per_worker_usage_in_vars);
    