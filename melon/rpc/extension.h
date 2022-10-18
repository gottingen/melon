// Licensed to the Apache Software Foundation (ASF) under one
// or more contributor license agreements.  See the NOTICE file
// distributed with this work for additional information
// regarding copyright ownership.  The ASF licenses this file
// to you under the Apache License, Version 2.0 (the
// "License"); you may not use this file except in compliance
// with the License.  You may obtain a copy of the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing,
// software distributed under the License is distributed on an
// "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied.  See the License for the
// specific language governing permissions and limitations
// under the License.


#ifndef MELON_RPC_EXTENSION_H_
#define MELON_RPC_EXTENSION_H_

#include <string>
#include "melon/base/scoped_lock.h"
#include "melon/log/logging.h"
#include "melon/container/case_ignored_flat_map.h"
#include "melon/base/singleton_on_pthread_once.h"

namespace melon::base {
    template<typename T>
    class GetLeakySingleton;
}


namespace melon::rpc {

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
        friend class melon::base::GetLeakySingleton<Extension<T> >;

        Extension();

        ~Extension();

        melon::container::CaseIgnoredFlatMap<T *> _instance_map;
        std::mutex _map_mutex;
    };

} // namespace melon::rpc


#include "melon/rpc/extension_inl.h"

#endif  // MELON_RPC_EXTENSION_H_
