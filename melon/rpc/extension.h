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



#ifndef MELON_RPC_EXTENSION_H_
#define MELON_RPC_EXTENSION_H_

#include <string>
#include "melon/utility/scoped_lock.h"
#include "melon/utility/logging.h"
#include "melon/utility/containers/case_ignored_flat_map.h"
#include "melon/utility/memory/singleton_on_pthread_once.h"

namespace mutil {
    template<typename T>
    class GetLeakySingleton;
}


namespace melon {

    // A global map from string to user-extended instances (typed T).
    // It's used by NamingService and LoadBalancer to maintain globally
    // available instances.
    // All names are case-insensitive. Names are printed in lowercases.

    template<typename T>
    class Extension {
    public:
        static Extension<T> *instance();

        int Register(const std::string &name, T *instance);

        int RegisterOrDie(const std::string &name, T *instance);

        T *Find(const char *name);

        void List(std::ostream &os, char separator);

    private:
        friend class mutil::GetLeakySingleton<Extension<T> >;

        Extension();

        ~Extension();

        mutil::CaseIgnoredFlatMap<T *> _instance_map;
        mutil::Mutex _map_mutex;
    };

} // namespace melon


#include "melon/rpc/extension_inl.h"

#endif  // MELON_RPC_EXTENSION_H_
