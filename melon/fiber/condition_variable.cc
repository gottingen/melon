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

#include "melon/utility/atomicops.h"
#include "melon/utility/macros.h"                         // MELON_CASSERT
#include "melon/fiber/butex.h"                       // butex_*
#include "melon/fiber/types.h"                       // fiber_cond_t

namespace fiber {
struct CondInternal {
    mutil::atomic<fiber_mutex_t*> m;
    mutil::atomic<int>* seq;
};

MELON_CASSERT(sizeof(CondInternal) == sizeof(fiber_cond_t),
              sizeof_innercond_must_equal_cond);
MELON_CASSERT(offsetof(CondInternal, m) == offsetof(fiber_cond_t, m),
              offsetof_cond_mutex_must_equal);
MELON_CASSERT(offsetof(CondInternal, seq) ==
              offsetof(fiber_cond_t, seq),
              offsetof_cond_seq_must_equal);
}

extern "C" {

extern int fiber_mutex_unlock(fiber_mutex_t*);
extern int fiber_mutex_lock_contended(fiber_mutex_t*);

int fiber_cond_init(fiber_cond_t* __restrict c,
                      const fiber_condattr_t*) {
    c->m = NULL;
    c->seq = fiber::butex_create_checked<int>();
    *c->seq = 0;
    return 0;
}

int fiber_cond_destroy(fiber_cond_t* c) {
    fiber::butex_destroy(c->seq);
    c->seq = NULL;
    return 0;
}

int fiber_cond_signal(fiber_cond_t* c) {
    fiber::CondInternal* ic = reinterpret_cast<fiber::CondInternal*>(c);
    // ic is probably dereferenced after fetch_add, save required fields before
    // this point
    mutil::atomic<int>* const saved_seq = ic->seq;
    saved_seq->fetch_add(1, mutil::memory_order_release);
    // don't touch ic any more
    fiber::butex_wake(saved_seq);
    return 0;
}

int fiber_cond_broadcast(fiber_cond_t* c) {
    fiber::CondInternal* ic = reinterpret_cast<fiber::CondInternal*>(c);
    fiber_mutex_t* m = ic->m.load(mutil::memory_order_relaxed);
    mutil::atomic<int>* const saved_seq = ic->seq;
    if (!m) {
        return 0;
    }
    void* const saved_butex = m->butex;
    // Wakeup one thread and requeue the rest on the mutex.
    ic->seq->fetch_add(1, mutil::memory_order_release);
    fiber::butex_requeue(saved_seq, saved_butex);
    return 0;
}

int fiber_cond_wait(fiber_cond_t* __restrict c,
                      fiber_mutex_t* __restrict m) {
    fiber::CondInternal* ic = reinterpret_cast<fiber::CondInternal*>(c);
    const int expected_seq = ic->seq->load(mutil::memory_order_relaxed);
    if (ic->m.load(mutil::memory_order_relaxed) != m) {
        // bind m to c
        fiber_mutex_t* expected_m = NULL;
        if (!ic->m.compare_exchange_strong(
                expected_m, m, mutil::memory_order_relaxed)) {
            return EINVAL;
        }
    }
    fiber_mutex_unlock(m);
    int rc1 = 0;
    if (fiber::butex_wait(ic->seq, expected_seq, NULL) < 0 &&
        errno != EWOULDBLOCK && errno != EINTR/*note*/) {
        // EINTR should not be returned by cond_*wait according to docs on
        // pthread, however spurious wake-up is OK, just as we do here
        // so that users can check flags in the loop often companioning
        // with the cond_wait ASAP. For example:
        //   mutex.lock();
        //   while (!stop && other-predicates) {
        //     cond_wait(&mutex);
        //   }
        //   mutex.unlock();
        // After interruption, above code should wake up from the cond_wait
        // soon and check the `stop' flag and other predicates.
        rc1 = errno;
    }
    const int rc2 = fiber_mutex_lock_contended(m);
    return (rc2 ? rc2 : rc1);
}

int fiber_cond_timedwait(fiber_cond_t* __restrict c,
                           fiber_mutex_t* __restrict m,
                           const struct timespec* __restrict abstime) {
    fiber::CondInternal* ic = reinterpret_cast<fiber::CondInternal*>(c);
    const int expected_seq = ic->seq->load(mutil::memory_order_relaxed);
    if (ic->m.load(mutil::memory_order_relaxed) != m) {
        // bind m to c
        fiber_mutex_t* expected_m = NULL;
        if (!ic->m.compare_exchange_strong(
                expected_m, m, mutil::memory_order_relaxed)) {
            return EINVAL;
        }
    }
    fiber_mutex_unlock(m);
    int rc1 = 0;
    if (fiber::butex_wait(ic->seq, expected_seq, abstime) < 0 &&
        errno != EWOULDBLOCK && errno != EINTR/*note*/) {
        // note: see comments in fiber_cond_wait on EINTR.
        rc1 = errno;
    }
    const int rc2 = fiber_mutex_lock_contended(m);
    return (rc2 ? rc2 : rc1);
}

}  // extern "C"
