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

#include <melon/utility/unique_ptr.h>           // std::unique_ptr


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
