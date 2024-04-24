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


#include "melon/raft/fsync.h"
#include <melon/rpc/reloadable_flags.h>  //MELON_VALIDATE_GFLAG

namespace melon::raft {

    DEFINE_bool(raft_use_fsync_rather_than_fdatasync,
                true,
                "Use fsync rather than fdatasync to flush page cache");

    MELON_VALIDATE_GFLAG(raft_use_fsync_rather_than_fdatasync,
                        melon::PassValidate);

}  //  namespace melon::raft
