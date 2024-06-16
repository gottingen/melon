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

namespace fiber {

// Utilities to manipulate fiber_t
    inline fiber_t make_tid(uint32_t version, mutil::ResourceId<TaskMeta> slot) {
        return (((fiber_t) version) << 32) | (fiber_t) slot.value;
    }

    inline mutil::ResourceId<TaskMeta> get_slot(fiber_t tid) {
        mutil::ResourceId<TaskMeta> id = {(tid & 0xFFFFFFFFul)};
        return id;
    }

    inline uint32_t get_version(fiber_t tid) {
        return (uint32_t) ((tid >> 32) & 0xFFFFFFFFul);
    }

    inline TaskMeta *TaskGroup::address_meta(fiber_t tid) {
        // TaskMeta * m = address_resource<TaskMeta>(get_slot(tid));
        // if (m != NULL && m->version == get_version(tid)) {
        //     return m;
        // }
        // return NULL;
        return address_resource(get_slot(tid));
    }

    inline void TaskGroup::exchange(TaskGroup **pg, fiber_t next_tid) {
        TaskGroup *g = *pg;
        if (g->is_current_pthread_task()) {
            return g->ready_to_run(next_tid);
        }
        ReadyToRunArgs args = {g->current_tid(), false};
        g->set_remained((g->current_task()->about_to_quit
                         ? ready_to_run_in_worker_ignoresignal
                         : ready_to_run_in_worker),
                        &args);
        TaskGroup::sched_to(pg, next_tid);
    }

    inline void TaskGroup::sched_to(TaskGroup **pg, fiber_t next_tid) {
        TaskMeta *next_meta = address_meta(next_tid);
        if (next_meta->stack == NULL) {
            ContextualStack *stk = get_stack(next_meta->stack_type(), task_runner);
            if (stk) {
                next_meta->set_stack(stk);
            } else {
                // stack_type is FIBER_STACKTYPE_PTHREAD or out of memory,
                // In latter case, attr is forced to be FIBER_STACKTYPE_PTHREAD.
                // This basically means that if we can't allocate stack, run
                // the task in pthread directly.
                next_meta->attr.stack_type = FIBER_STACKTYPE_PTHREAD;
                next_meta->set_stack((*pg)->_main_stack);
            }
        }
        // Update now_ns only when wait_task did yield.
        sched_to(pg, next_meta);
    }

    inline void TaskGroup::push_rq(fiber_t tid) {
        while (!_rq.push(tid)) {
            // Created too many fibers: a promising approach is to insert the
            // task into another TaskGroup, but we don't use it because:
            // * There're already many fibers to run, inserting the fiber
            //   into other TaskGroup does not help.
            // * Insertions into other TaskGroups perform worse when all workers
            //   are busy at creating fibers (proved by test_input_messenger in
            //   melon)
            flush_nosignal_tasks();
            LOG_EVERY_N_SEC(ERROR, 1) << "_rq is full, capacity=" << _rq.capacity();
            // TODO(gejun): May cause deadlock when all workers are spinning here.
            // A better solution is to pop and run existing fibers, however which
            // make set_remained()-callbacks do context switches and need extensive
            // reviews on related code.
            ::usleep(1000);
        }
    }

    inline void TaskGroup::flush_nosignal_tasks_remote() {
        if (_remote_num_nosignal) {
            _remote_rq._mutex.lock();
            flush_nosignal_tasks_remote_locked(_remote_rq._mutex);
        }
    }

}  // namespace fiber

