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

#pragma once

#include "melon/utility/object_pool.h"
#include "melon/rpc/input_messenger.h"


namespace melon {
namespace policy {

// Try to use this message as the intermediate message between Parse() and
// Process() to maximize usage of ObjectPool<MostCommonMessage>, otherwise
// you have to new the messages or use a separate ObjectPool (which is likely
// to waste more memory)
struct MELON_CACHELINE_ALIGNMENT MostCommonMessage : public InputMessageBase {
    mutil::IOBuf meta;
    mutil::IOBuf payload;
    PipelinedInfo pi;

    inline static MostCommonMessage* Get() {
        return mutil::get_object<MostCommonMessage>();
    }

    // @InputMessageBase
    void DestroyImpl() {
        meta.clear();
        payload.clear();
        pi.reset();
        mutil::return_object(this);
    }
};

}  // namespace policy
} // namespace melon
