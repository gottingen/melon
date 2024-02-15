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


#ifndef MELON_FIBER_TASK_META_H_
#define MELON_FIBER_TASK_META_H_

#include <pthread.h>                 // pthread_spin_init
#include "melon/fiber/butex.h"           // butex_construct/destruct
#include "melon/utility/atomicops.h"          // mutil::atomic
#include "melon/fiber/types.h"           // fiber_attr_t
#include "melon/fiber/stack.h"           // ContextualStack

namespace fiber {

struct TaskStatistics {
    int64_t cputime_ns;
    int64_t nswitch;
};

class KeyTable;
struct ButexWaiter;

struct LocalStorage {
    KeyTable* keytable;
    void* assigned_data;
    void* rpcz_parent_span;
};

#define FIBER_LOCAL_STORAGE_INITIALIZER { NULL, NULL, NULL }

const static LocalStorage LOCAL_STORAGE_INIT = FIBER_LOCAL_STORAGE_INITIALIZER;

struct TaskMeta {
    // [Not Reset]
    mutil::atomic<ButexWaiter*> current_waiter;
    uint64_t current_sleep;

    // A builtin flag to mark if the thread is stopping.
    bool stop;

    // The thread is interrupted and should wake up from some blocking ops.
    bool interrupted;

    // Scheduling of the thread can be delayed.
    bool about_to_quit;
    
    // [Not Reset] guarantee visibility of version_butex.
    pthread_spinlock_t version_lock;
    
    // [Not Reset] only modified by one fiber at any time, no need to be atomic
    uint32_t* version_butex;

    // The identifier. It does not have to be here, however many code is
    // simplified if they can get tid from TaskMeta.
    fiber_t tid;

    // User function and argument
    void* (*fn)(void*);
    void* arg;

    // Stack of this task.
    ContextualStack* stack;

    // Attributes creating this task
    fiber_attr_t attr;
    
    // Statistics
    int64_t cpuwide_start_ns;
    TaskStatistics stat;

    // fiber local storage, sync with tls_bls (defined in task_group.cpp)
    // when the fiber is created or destroyed.
    // DO NOT use this field directly, use tls_bls instead.
    LocalStorage local_storage;

public:
    // Only initialize [Not Reset] fields, other fields will be reset in
    // fiber_start* functions
    TaskMeta()
        : current_waiter(NULL)
        , current_sleep(0)
        , stack(NULL) {
        pthread_spin_init(&version_lock, 0);
        version_butex = butex_create_checked<uint32_t>();
        *version_butex = 1;
    }
        
    ~TaskMeta() {
        butex_destroy(version_butex);
        version_butex = NULL;
        pthread_spin_destroy(&version_lock);
    }

    void set_stack(ContextualStack* s) {
        stack = s;
    }

    ContextualStack* release_stack() {
        ContextualStack* tmp = stack;
        stack = NULL;
        return tmp;
    }

    StackType stack_type() const {
        return static_cast<StackType>(attr.stack_type);
    }
};

}  // namespace fiber

#endif  // MELON_FIBER_TASK_META_H_
