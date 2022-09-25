// Licensed to the Apache Software Foundation (ASF) under one
// or more contributor license agreements.  See the NOTICE file
// distributed with this work for additional information
// regarding copyright ownership.  The ASF licenses this file
// to you under the Apache License, Version 2.0 (the
// "License"); you may not use this file except in compliance
// with the License.  You may obtain a copy of the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing,
// software distributed under the License is distributed on an
// "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied.  See the License for the
// specific language governing permissions and limitations
// under the License.

// fiber - A M:N threading library to make applications more concurrent.

// Date: Tue Jul 10 17:40:58 CST 2012

#include <gflags/gflags.h>
#include <utility>
#include "melon/log/logging.h"
#include "melon/fiber/internal/fiber_worker.h"                // fiber_worker
#include "melon/fiber/internal/schedule_group.h"              // schedule_group
#include "melon/fiber/internal/timer_thread.h"
#include "melon/fiber/internal/list_of_abafree_id.h"
#include "melon/fiber/internal/fiber.h"

namespace melon::fiber_internal {

    DEFINE_int32(fiber_concurrency, 8 + FIBER_EPOLL_THREAD_NUM,
                 "Number of pthread workers");

    DEFINE_int32(fiber_min_concurrency, 0,
                 "Initial number of pthread workers which will be added on-demand."
                 " The laziness is disabled when this value is non-positive,"
                 " and workers will be created eagerly according to -fiber_concurrency and fiber_setconcurrency(). ");

    static bool never_set_fiber_concurrency = true;

    static bool validate_fiber_concurrency(const char *, int32_t val) {
        // fiber_setconcurrency sets the flag on success path which should
        // not be strictly in a validator. But it's OK for a int flag.
        return fiber_setconcurrency(val) == 0;
    }

    const int MELON_ALLOW_UNUSED register_FLAGS_fiber_concurrency =
            ::google::RegisterFlagValidator(&FLAGS_fiber_concurrency,
                                               validate_fiber_concurrency);

    static bool validate_fiber_min_concurrency(const char *, int32_t val);

    const int MELON_ALLOW_UNUSED register_FLAGS_fiber_min_concurrency =
            ::google::RegisterFlagValidator(&FLAGS_fiber_min_concurrency,
                                               validate_fiber_min_concurrency);

    static_assert(sizeof(schedule_group *) == sizeof(std::atomic<schedule_group *>), "atomic_size_match");

    pthread_mutex_t g_task_control_mutex = PTHREAD_MUTEX_INITIALIZER;
    // Referenced in rpc, needs to be extern.
    // Notice that we can't declare the variable as atomic<schedule_group*> which
    // are not constructed before main().
    schedule_group *g_task_control = NULL;

    extern MELON_THREAD_LOCAL fiber_worker *tls_task_group;

    extern void (*g_worker_startfn)();

    inline schedule_group *get_task_control() {
        return g_task_control;
    }

    inline schedule_group *get_or_new_task_control() {
        std::atomic<schedule_group *> *p = (std::atomic<schedule_group *> *) &g_task_control;
        schedule_group *c = p->load(std::memory_order_consume);
        if (c != NULL) {
            return c;
        }
        MELON_SCOPED_LOCK(g_task_control_mutex);
        c = p->load(std::memory_order_consume);
        if (c != NULL) {
            return c;
        }
        c = new(std::nothrow) schedule_group;
        if (NULL == c) {
            return NULL;
        }
        int concurrency = FLAGS_fiber_min_concurrency > 0 ?
                          FLAGS_fiber_min_concurrency :
                          FLAGS_fiber_concurrency;
        if (c->init(concurrency) != 0) {
            MELON_LOG(ERROR) << "Fail to init g_task_control";
            delete c;
            return NULL;
        }
        p->store(c, std::memory_order_release);
        return c;
    }

    static bool validate_fiber_min_concurrency(const char *, int32_t val) {
        if (val <= 0) {
            return true;
        }
        if (val < FIBER_MIN_CONCURRENCY || val > FLAGS_fiber_concurrency) {
            return false;
        }
        schedule_group *c = get_task_control();
        if (!c) {
            return true;
        }
        MELON_SCOPED_LOCK(g_task_control_mutex);
        int concurrency = c->concurrency();
        if (val > concurrency) {
            int added = c->add_workers(val - concurrency);
            return added == (val - concurrency);
        } else {
            return true;
        }
    }

    __thread fiber_worker *tls_task_group_nosignal = NULL;

    MELON_FORCE_INLINE int
    start_from_non_worker(fiber_id_t *__restrict tid,
                          const fiber_attribute *__restrict attr,
                          std::function<void *(void *)> &&fn,
                          void *__restrict arg) {
        schedule_group *c = get_or_new_task_control();
        if (NULL == c) {
            return ENOMEM;
        }
        if (attr != NULL && (attr->flags & FIBER_NOSIGNAL)) {
            // Remember the fiber_worker to insert NOSIGNAL tasks for 2 reasons:
            // 1. NOSIGNAL is often for creating many fibers in batch,
            //    inserting into the same fiber_worker maximizes the batch.
            // 2. fiber_flush() needs to know which fiber_worker to flush.
            fiber_worker *g = tls_task_group_nosignal;
            if (NULL == g) {
                g = c->choose_one_group();
                tls_task_group_nosignal = g;
            }
            return g->start_background<true>(tid, attr, std::move(fn), arg);
        }
        return c->choose_one_group()->start_background<true>(
                tid, attr, std::move(fn), arg);
    }

    struct TidTraits {
        static const size_t BLOCK_SIZE = 63;
        static const size_t MAX_ENTRIES = 65536;
        static const fiber_id_t TOKEN_INIT;

        static bool exists(fiber_id_t id) { return melon::fiber_internal::fiber_worker::exists(id); }
    };

    const fiber_id_t TidTraits::TOKEN_INIT = INVALID_FIBER_ID;

    typedef ListOfABAFreeId<fiber_id_t, TidTraits> TidList;

    struct TidStopper {
        void operator()(fiber_id_t id) const { fiber_stop(id); }
    };

    struct TidJoiner {
        void operator()(fiber_id_t &id) const {
            fiber_join(id, NULL);
            id = INVALID_FIBER_ID;
        }
    };

}  // namespace melon::fiber_internal

extern "C" {

int fiber_start_urgent(fiber_id_t *__restrict tid,
                       const fiber_attribute *__restrict attr,
                       std::function<void *(void *)> &&fn,
                       void *__restrict arg) {
    melon::fiber_internal::fiber_worker *g = melon::fiber_internal::tls_task_group;
    if (g) {
        // start from worker
        return melon::fiber_internal::fiber_worker::start_foreground(&g, tid, attr, std::move(fn), arg);
    }
    return melon::fiber_internal::start_from_non_worker(tid, attr, std::move(fn), arg);
}

int fiber_start_background(fiber_id_t *__restrict tid,
                           const fiber_attribute *__restrict attr,
                           std::function<void *(void *)> &&fn,
                           void *__restrict arg) {
    melon::fiber_internal::fiber_worker *g = melon::fiber_internal::tls_task_group;
    if (g) {
        // start from worker
        return g->start_background<false>(tid, attr, std::move(fn), arg);
    }
    return melon::fiber_internal::start_from_non_worker(tid, attr, std::move(fn), arg);
}

void fiber_flush() {
    melon::fiber_internal::fiber_worker *g = melon::fiber_internal::tls_task_group;
    if (g) {
        return g->flush_nosignal_tasks();
    }
    g = melon::fiber_internal::tls_task_group_nosignal;
    if (g) {
        // NOSIGNAL tasks were created in this non-worker.
        melon::fiber_internal::tls_task_group_nosignal = NULL;
        return g->flush_nosignal_tasks_remote();
    }
}

int fiber_interrupt(fiber_id_t tid) {
    return melon::fiber_internal::fiber_worker::interrupt(tid, melon::fiber_internal::get_task_control());
}

int fiber_stop(fiber_id_t tid) {
    melon::fiber_internal::fiber_worker::set_stopped(tid);
    return fiber_interrupt(tid);
}

int fiber_stopped(fiber_id_t tid) {
    return (int) melon::fiber_internal::fiber_worker::is_stopped(tid);
}

fiber_id_t fiber_self(void) {
    melon::fiber_internal::fiber_worker *g = melon::fiber_internal::tls_task_group;
    // note: return 0 for main tasks now, which include main thread and
    // all work threads. So that we can identify main tasks from logs
    // more easily. This is probably questionable in future.
    if (g != NULL && !g->is_current_main_task()/*note*/) {
        return g->current_fid();
    }
    return INVALID_FIBER_ID;
}

int fiber_equal(fiber_id_t t1, fiber_id_t t2) {
    return t1 == t2;
}

void fiber_exit(void *retval) {
    melon::fiber_internal::fiber_worker *g = melon::fiber_internal::tls_task_group;
    if (g != NULL && !g->is_current_main_task()) {
        throw melon::fiber_internal::ExitException(retval);
    } else {
        pthread_exit(retval);
    }
}

int fiber_join(fiber_id_t tid, void **thread_return) {
    return melon::fiber_internal::fiber_worker::join(tid, thread_return);
}

int fiber_attr_init(fiber_attribute *a) {
    *a = FIBER_ATTR_NORMAL;
    return 0;
}

int fiber_attr_destroy(fiber_attribute *) {
    return 0;
}

int fiber_getattr(fiber_id_t tid, fiber_attribute *attr) {
    return melon::fiber_internal::fiber_worker::get_attr(tid, attr);
}

int fiber_getconcurrency(void) {
    return melon::fiber_internal::FLAGS_fiber_concurrency;
}

int fiber_setconcurrency(int num) {
    if (num < FIBER_MIN_CONCURRENCY || num > FIBER_MAX_CONCURRENCY) {
        MELON_LOG(ERROR) << "Invalid concurrency=" << num;
        return EINVAL;
    }
    if (melon::fiber_internal::FLAGS_fiber_min_concurrency > 0) {
        if (num < melon::fiber_internal::FLAGS_fiber_min_concurrency) {
            return EINVAL;
        }
        if (melon::fiber_internal::never_set_fiber_concurrency) {
            melon::fiber_internal::never_set_fiber_concurrency = false;
        }
        melon::fiber_internal::FLAGS_fiber_concurrency = num;
        return 0;
    }
    melon::fiber_internal::schedule_group *c = melon::fiber_internal::get_task_control();
    if (c != NULL) {
        if (num < c->concurrency()) {
            return EPERM;
        } else if (num == c->concurrency()) {
            return 0;
        }
    }
    MELON_SCOPED_LOCK(melon::fiber_internal::g_task_control_mutex);
    c = melon::fiber_internal::get_task_control();
    if (c == NULL) {
        if (melon::fiber_internal::never_set_fiber_concurrency) {
            melon::fiber_internal::never_set_fiber_concurrency = false;
            melon::fiber_internal::FLAGS_fiber_concurrency = num;
        } else if (num > melon::fiber_internal::FLAGS_fiber_concurrency) {
            melon::fiber_internal::FLAGS_fiber_concurrency = num;
        }
        return 0;
    }
    if (melon::fiber_internal::FLAGS_fiber_concurrency != c->concurrency()) {
        MELON_LOG(ERROR) << "MELON_CHECK failed: fiber_concurrency="
                   << melon::fiber_internal::FLAGS_fiber_concurrency
                   << " != tc_concurrency=" << c->concurrency();
        melon::fiber_internal::FLAGS_fiber_concurrency = c->concurrency();
    }
    if (num > melon::fiber_internal::FLAGS_fiber_concurrency) {
        // Create more workers if needed.
        melon::fiber_internal::FLAGS_fiber_concurrency +=
                c->add_workers(num - melon::fiber_internal::FLAGS_fiber_concurrency);
        return 0;
    }
    return (num == melon::fiber_internal::FLAGS_fiber_concurrency ? 0 : EPERM);
}

int fiber_about_to_quit() {
    melon::fiber_internal::fiber_worker *g = melon::fiber_internal::tls_task_group;
    if (g != NULL) {
        melon::fiber_internal::fiber_entity *current_task = g->current_task();
        if (!(current_task->attr.flags & FIBER_NEVER_QUIT)) {
            current_task->about_to_quit = true;
        }
        return 0;
    }
    return EPERM;
}

int fiber_timer_add(fiber_timer_id *id, timespec abstime,
                    void (*on_timer)(void *), void *arg) {
    melon::fiber_internal::schedule_group *c = melon::fiber_internal::get_or_new_task_control();
    if (c == NULL) {
        return ENOMEM;
    }
    melon::fiber_internal::TimerThread *tt = melon::fiber_internal::get_or_create_global_timer_thread();
    if (tt == NULL) {
        return ENOMEM;
    }
    fiber_timer_id tmp = tt->schedule(on_timer, arg, abstime);
    if (tmp != 0) {
        *id = tmp;
        return 0;
    }
    return ESTOP;
}

int fiber_timer_del(fiber_timer_id id) {
    melon::fiber_internal::schedule_group *c = melon::fiber_internal::get_task_control();
    if (c != NULL) {
        melon::fiber_internal::TimerThread *tt = melon::fiber_internal::get_global_timer_thread();
        if (tt == NULL) {
            return EINVAL;
        }
        const int state = tt->unschedule(id);
        if (state >= 0) {
            return state;
        }
    }
    return EINVAL;
}

int fiber_set_worker_startfn(void (*start_fn)()) {
    if (start_fn == NULL) {
        return EINVAL;
    }
    melon::fiber_internal::g_worker_startfn = start_fn;
    return 0;
}

void fiber_stop_world() {
    melon::fiber_internal::schedule_group *c = melon::fiber_internal::get_task_control();
    if (c != NULL) {
        c->stop_and_join();
    }
}

int fiber_list_init(fiber_list_t *list,
                    unsigned /*size*/,
                    unsigned /*conflict_size*/) {
    list->impl = new(std::nothrow) melon::fiber_internal::TidList;
    if (NULL == list->impl) {
        return ENOMEM;
    }
    // Set unused fields to zero as well.
    list->head = 0;
    list->size = 0;
    list->conflict_head = 0;
    list->conflict_size = 0;
    return 0;
}

void fiber_list_destroy(fiber_list_t *list) {
    delete static_cast<melon::fiber_internal::TidList *>(list->impl);
    list->impl = NULL;
}

int fiber_list_add(fiber_list_t *list, fiber_id_t id) {
    if (list->impl == NULL) {
        return EINVAL;
    }
    return static_cast<melon::fiber_internal::TidList *>(list->impl)->add(id);
}

int fiber_list_stop(fiber_list_t *list) {
    if (list->impl == NULL) {
        return EINVAL;
    }
    static_cast<melon::fiber_internal::TidList *>(list->impl)->apply(melon::fiber_internal::TidStopper());
    return 0;
}

int fiber_list_join(fiber_list_t *list) {
    if (list->impl == NULL) {
        return EINVAL;
    }
    static_cast<melon::fiber_internal::TidList *>(list->impl)->apply(melon::fiber_internal::TidJoiner());
    return 0;
}

}  // extern "C"
