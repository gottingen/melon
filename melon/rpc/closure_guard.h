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

#include <google/protobuf/service.h>
#include <melon/base/macros.h>


namespace melon {

    // RAII: Call Run() of the closure on destruction.
    class ClosureGuard {
    public:
        ClosureGuard() : _done(NULL) {}

        // Constructed with a closure which will be Run() inside dtor.
        explicit ClosureGuard(google::protobuf::Closure *done) : _done(done) {}

        // Run internal closure if it's not NULL.
        ~ClosureGuard() {
            if (_done) {
                _done->Run();
            }
        }

        // Run internal closure if it's not NULL and set it to `done'.
        void reset(google::protobuf::Closure *done) {
            if (_done) {
                _done->Run();
            }
            _done = done;
        }

        // Return and set internal closure to NULL.
        google::protobuf::Closure *release() {
            google::protobuf::Closure *const prev_done = _done;
            _done = NULL;
            return prev_done;
        }

        // True if no closure inside.
        bool empty() const { return _done == NULL; }

        // Exchange closure with another guard.
        void swap(ClosureGuard &other) { std::swap(_done, other._done); }

    private:
        // Copying this object makes no sense.
        DISALLOW_COPY_AND_ASSIGN(ClosureGuard);

        google::protobuf::Closure *_done;
    };

} // namespace melon
