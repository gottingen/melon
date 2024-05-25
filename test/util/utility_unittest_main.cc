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


#include <sys/resource.h>
#include <gtest/gtest.h>
#include <gflags/gflags.h>
#include "melon/utility/at_exit.h"
#include <melon/utility/logging.h>
#include "multiprocess_func_list.h"

DEFINE_bool(disable_coredump, false, "Never core dump");

int main(int argc, char** argv) {
    mutil::AtExitManager at_exit;
    testing::InitGoogleTest(&argc, argv);
    
    google::ParseCommandLineFlags(&argc, &argv, true);
    if (FLAGS_disable_coredump) {
        rlimit core_limit;
        core_limit.rlim_cur = 0;
        core_limit.rlim_max = 0;
        setrlimit(RLIMIT_CORE, &core_limit);
    }
#if !MELON_WITH_GLOG
    MCHECK(!google::SetCommandLineOption("crash_on_fatal_log", "true").empty());
#endif
    return RUN_ALL_TESTS();
}
