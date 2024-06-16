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

#include <string>
#include <melon/base/scoped_lock.h>
#include <turbo/log/logging.h>
#include <melon/base/containers/case_ignored_flat_map.h>
#include <melon/utility/memory/singleton_on_pthread_once.h>

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


#include <melon/rpc/extension_inl.h>
