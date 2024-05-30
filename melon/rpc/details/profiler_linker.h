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

#if defined(MELON_ENABLE_CPU_PROFILER) || defined(MELON_RPC_ENABLE_CPU_PROFILER)
#include <melon/utility/gperftools_profiler.h>
#endif

namespace melon {

    // defined in src/melon/builtin/index_service.cpp
    extern bool cpu_profiler_enabled;

    // defined in src/melon/controller.cpp
    extern int PROFILER_LINKER_DUMMY;

    struct ProfilerLinker {
        // [ Must be inlined ]
        // This function is included by user's compilation unit to force
        // linking of ProfilerStart()/ProfilerStop()
        // etc when corresponding macros are defined.
        inline ProfilerLinker() {

#if defined(MELON_ENABLE_CPU_PROFILER) || defined(MELON_RPC_ENABLE_CPU_PROFILER)
            cpu_profiler_enabled = true;
            // compiler has no way to tell if PROFILER_LINKER_DUMMY is 0 or not,
            // so it has to link the function inside the branch.
            if (PROFILER_LINKER_DUMMY != 0/*must be false*/) {
                ProfilerStart("this_function_should_never_run");
            }
#endif
        }
    };

} // namespace melon
