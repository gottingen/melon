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



#ifndef MELON_RPC_DESTROYABLE_H_
#define MELON_RPC_DESTROYABLE_H_

#include "melon/utility/unique_ptr.h"           // std::unique_ptr


namespace melon {

    class Destroyable {
    public:
        virtual ~Destroyable() {}

        virtual void Destroy() = 0;
    };

    namespace detail {
        template<typename T>
        struct Destroyer {
            void operator()(T *obj) const { if (obj) { obj->Destroy(); }}
        };
    }

    // A special unique_ptr that calls "obj->Destroy()" instead of "delete obj".
    template<typename T>
    struct DestroyingPtr : public std::unique_ptr<T, detail::Destroyer<T> > {
        DestroyingPtr() {}

        DestroyingPtr(T *p) : std::unique_ptr<T, detail::Destroyer<T> >(p) {}
    };

} // namespace melon


#endif  // MELON_RPC_DESTROYABLE_H_
