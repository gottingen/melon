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

#include <melon/base/scoped_lock.h>
#include <melon/utility/containers/bounded_queue.h>
#include <melon/rpc/socket_id.h>                       // SocketId


namespace melon {

    // Remember servers that should be avoided in selection. These servers
    // are often selected in previous tries inside a RPC.
    class ExcludedServers {
    public:
        // Create a instance with at most `cap' servers.
        static ExcludedServers *Create(int cap);

        // Destroy the instance
        static void Destroy(ExcludedServers *ptr);

        // Add a server. If the internal queue is full, pop one from the queue first.
        void Add(SocketId id);

        // True if the server shall be excluded.
        bool IsExcluded(SocketId id) const;

        static bool IsExcluded(const ExcludedServers *s, SocketId id) {
            return s != NULL && s->IsExcluded(id);
        }

        // #servers inside.
        size_t size() const { return _l.size(); }

    private:
        ExcludedServers(int cap)
                : _l(_space, sizeof(SocketId) * cap, mutil::NOT_OWN_STORAGE) {}

        ~ExcludedServers() {}

        // Controller::_accessed may be shared by sub channels in schan, protect
        // all mutable methods with this mutex. In ordinary channels, this mutex
        // is never contended.
        mutable mutil::Mutex _mutex;
        mutil::BoundedQueue<SocketId> _l;
        SocketId _space[0];
    };

    // ===================================================

    inline ExcludedServers *ExcludedServers::Create(int cap) {
        void *space = malloc(
                offsetof(ExcludedServers, _space) + sizeof(SocketId) * cap);
        if (NULL == space) {
            return NULL;
        }
        return new(space) ExcludedServers(cap);
    }

    inline void ExcludedServers::Destroy(ExcludedServers *ptr) {
        if (ptr) {
            ptr->~ExcludedServers();
            free(ptr);
        }
    }

    inline void ExcludedServers::Add(SocketId id) {
        MELON_SCOPED_LOCK(_mutex);
        const SocketId *last_id = _l.bottom();
        if (last_id == NULL || *last_id != id) {
            _l.elim_push(id);
        }
    }

    inline bool ExcludedServers::IsExcluded(SocketId id) const {
        MELON_SCOPED_LOCK(_mutex);
        for (size_t i = 0; i < _l.size(); ++i) {
            if (*_l.bottom(i) == id) {
                return true;
            }
        }
        return false;
    }

} // namespace melon
