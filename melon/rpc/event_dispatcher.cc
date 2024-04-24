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



#include <gflags/gflags.h>                            // DEFINE_int32
#include "melon/utility/compat.h"
#include "melon/utility/fd_utility.h"                         // make_close_on_exec
#include "melon/utility/logging.h"                            // LOG
#include "melon/utility/third_party/murmurhash3/murmurhash3.h"// fmix32
#include "melon/fiber/fiber.h"                          // fiber_start_background
#include "melon/rpc/event_dispatcher.h"
#include "melon/rpc/reloadable_flags.h"

DECLARE_int32(task_group_ntags);

namespace melon {

DEFINE_int32(event_dispatcher_num, 1, "Number of event dispatcher");

DEFINE_bool(usercode_in_pthread, false, 
            "Call user's callback in pthreads, use fibers otherwise");
DEFINE_bool(usercode_in_coroutine, false,
            "User's callback are run in coroutine, no fiber or pthread blocking call");

static EventDispatcher* g_edisp = NULL;
static pthread_once_t g_edisp_once = PTHREAD_ONCE_INIT;

static void StopAndJoinGlobalDispatchers() {
    for (int i = 0; i < FLAGS_task_group_ntags; ++i) {
        for (int j = 0; j < FLAGS_event_dispatcher_num; ++j) {
            g_edisp[i * FLAGS_event_dispatcher_num + j].Stop();
            g_edisp[i * FLAGS_event_dispatcher_num + j].Join();
        }
    }
}
void InitializeGlobalDispatchers() {
    g_edisp = new EventDispatcher[FLAGS_task_group_ntags * FLAGS_event_dispatcher_num];
    for (int i = 0; i < FLAGS_task_group_ntags; ++i) {
        for (int j = 0; j < FLAGS_event_dispatcher_num; ++j) {
            fiber_attr_t attr =
                FLAGS_usercode_in_pthread ? FIBER_ATTR_PTHREAD : FIBER_ATTR_NORMAL;
            attr.tag = (FIBER_TAG_DEFAULT + i) % FLAGS_task_group_ntags;
            MCHECK_EQ(0, g_edisp[i * FLAGS_event_dispatcher_num + j].Start(&attr));
        }
    }
    // This atexit is will be run before g_task_control.stop() because above
    // Start() initializes g_task_control by creating fiber (to run epoll/kqueue).
    MCHECK_EQ(0, atexit(StopAndJoinGlobalDispatchers));
}

EventDispatcher& GetGlobalEventDispatcher(int fd, fiber_tag_t tag) {
    pthread_once(&g_edisp_once, InitializeGlobalDispatchers);
    if (FLAGS_task_group_ntags == 1 && FLAGS_event_dispatcher_num == 1) {
        return g_edisp[0];
    }
    int index = mutil::fmix32(fd) % FLAGS_event_dispatcher_num;
    return g_edisp[tag * FLAGS_event_dispatcher_num + index];
}

} // namespace melon

#if defined(OS_LINUX)
    #include "melon/rpc/event_dispatcher_epoll.cc"
#elif defined(OS_MACOSX)
    #include "melon/rpc/event_dispatcher_kqueue.cc"
#else
    #error Not implemented
#endif
