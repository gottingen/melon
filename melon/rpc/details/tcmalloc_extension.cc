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


#include <pthread.h>
#include <dlfcn.h>                               // dlsym
#include <stdlib.h>                              // getenv
#include <melon/base/compiler_specific.h>
#include <melon/rpc/details/tcmalloc_extension.h>

namespace {
typedef MallocExtension* (*GetInstanceFn)();
static pthread_once_t g_get_instance_fn_once = PTHREAD_ONCE_INIT;
static GetInstanceFn g_get_instance_fn = NULL;
static void InitGetInstanceFn() {
    g_get_instance_fn = (GetInstanceFn)dlsym(
        RTLD_NEXT, "_ZN15MallocExtension8instanceEv");
}
} // namespace

MallocExtension* MELON_WEAK MallocExtension::instance() {
    // On fedora 26, this weak function is NOT overriden by the one in tcmalloc
    // which is dynamically linked.The same issue can't be re-produced in
    // Ubuntu and the exact cause is unknown yet. Using dlsym to get the
    // function works around the issue right now. Note that we can't use dlsym
    // to fully replace the weak-function mechanism since our code are generally
    // not compiled with -rdynamic which writes symbols to the table that
    // dlsym reads.
    pthread_once(&g_get_instance_fn_once, InitGetInstanceFn);
    if (g_get_instance_fn) {
        return g_get_instance_fn();
    }
    return NULL;
}

bool IsHeapProfilerEnabled() {
    return MallocExtension::instance() != NULL;
}

bool IsTCMallocEnabled() {
    return IsHeapProfilerEnabled();
}

static bool check_TCMALLOC_SAMPLE_PARAMETER() {
    char* str = getenv("TCMALLOC_SAMPLE_PARAMETER");
    if (str == NULL) {
        return false;
    }
    char* endptr;
    int val = strtol(str, &endptr, 10);
    return (*endptr == '\0' && val > 0);
}

bool has_TCMALLOC_SAMPLE_PARAMETER() {
    static bool val = check_TCMALLOC_SAMPLE_PARAMETER();
    return val;
}
