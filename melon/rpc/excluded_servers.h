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


#ifndef MELON_RPC_EXCLUDED_SERVERS_H_
#define MELON_RPC_EXCLUDED_SERVERS_H_

#include "melon/base/scoped_lock.h"
#include "melon/container/bounded_queue.h"
#include "melon/rpc/socket_id.h"                       // SocketId


namespace melon::rpc {

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
            return s != nullptr && s->IsExcluded(id);
        }

        // #servers inside.
        size_t size() const { return _l.size(); }

    private:

        ExcludedServers(int cap)
                : _l(_space, sizeof(SocketId) * cap, melon::container::NOT_OWN_STORAGE) {}

        ~ExcludedServers() {}

        // Controller::_accessed may be shared by sub channels in schan, protect
        // all mutable methods with this mutex. In ordinary channels, this mutex
        // is never contended.
        mutable std::mutex _mutex;
        melon::container::bounded_queue<SocketId> _l;
        SocketId _space[0];
    };

    // ===================================================

    inline ExcludedServers *ExcludedServers::Create(int cap) {
        void *space = malloc(
                offsetof(ExcludedServers, _space) + sizeof(SocketId) * cap);
        if (nullptr == space) {
            return nullptr;
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
        if (last_id == nullptr || *last_id != id) {
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

} // namespace melon::rpc


#endif  // MELON_RPC_EXCLUDED_SERVERS_H_
