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

#include <melon/utility/macros.h>                     // DISALLOW_COPY_AND_ASSIGN
#include <melon/fiber/types.h>                   // fiber_t, fiber_attr_t
#include <melon/rpc/socket.h>                     // Socket, SocketId


namespace melon {

// Dispatch edge-triggered events of file descriptors to consumers
// running in separate fibers.
    class EventDispatcher {
        friend class Socket;

        friend class rdma::RdmaEndpoint;

    public:
        EventDispatcher();

        virtual ~EventDispatcher();

        // Start this dispatcher in a fiber.
        // Use |*consumer_thread_attr| (if it's not NULL) as the attribute to
        // create fibers running user callbacks.
        // Returns 0 on success, -1 otherwise.
        virtual int Start(const fiber_attr_t *consumer_thread_attr);

        // True iff this dispatcher is running in a fiber
        bool Running() const;

        // Stop fiber of this dispatcher.
        void Stop();

        // Suspend calling thread until fiber of this dispatcher stops.
        void Join();

        // When edge-triggered events happen on `fd', call
        // `on_edge_triggered_events' of `socket_id'.
        // Notice that this function also transfers ownership of `socket_id',
        // When the file descriptor is removed from internal epoll, the Socket
        // will be dereferenced once additionally.
        // Returns 0 on success, -1 otherwise.
        int AddConsumer(SocketId socket_id, int fd);

        // Watch EPOLLOUT event on `fd' into epoll device. If `pollin' is
        // true, EPOLLIN event will also be included and EPOLL_CTL_MOD will
        // be used instead of EPOLL_CTL_ADD. When event arrives,
        // `Socket::HandleEpollOut' will be called with `socket_id'
        // Returns 0 on success, -1 otherwise and errno is set
        int AddEpollOut(SocketId socket_id, int fd, bool pollin);

        // Remove EPOLLOUT event on `fd'. If `pollin' is true, EPOLLIN event
        // will be kept and EPOLL_CTL_MOD will be used instead of EPOLL_CTL_DEL
        // Returns 0 on success, -1 otherwise and errno is set
        int RemoveEpollOut(SocketId socket_id, int fd, bool pollin);

    private:
        DISALLOW_COPY_AND_ASSIGN(EventDispatcher);

        // Calls Run()
        static void *RunThis(void *arg);

        // Thread entry.
        void Run();

        // Remove the file descriptor `fd' from epoll.
        int RemoveConsumer(int fd);

        // The epoll to watch events.
        int _epfd;

        // false unless Stop() is called.
        volatile bool _stop;

        // identifier of hosting fiber
        fiber_t _tid;

        // The attribute of fibers calling user callbacks.
        fiber_attr_t _consumer_thread_attr;

        // Pipe fds to wakeup EventDispatcher from `epoll_wait' in order to quit
        int _wakeup_fds[2];
    };

    EventDispatcher &GetGlobalEventDispatcher(int fd, fiber_tag_t tag);

} // namespace melon

