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

#include <melon/fiber/fiber.h>                       // fiber_t
#include <melon/utility/synchronization/condition_variable.h>
#include <melon/utility/containers/flat_map.h>
#include <melon/rpc/input_messenger.h>


namespace melon {

struct ConnectStatistics {
};

// Accept connections from a specific port and then
// process messages from which it reads
class Acceptor : public InputMessenger {
friend class Server;
public:
    typedef mutil::FlatMap<SocketId, ConnectStatistics> SocketMap;

    enum Status {
        UNINITIALIZED = 0,
        READY = 1,
        RUNNING = 2,
        STOPPING = 3,
    };

public:
    explicit Acceptor(fiber_keytable_pool_t* pool = NULL);
    ~Acceptor();

    // [thread-safe] Accept connections from `listened_fd'. Ownership of
    // `listened_fd' is also transferred to `Acceptor'. Can be called
    // multiple times if the last `StartAccept' has been completely stopped
    // by calling `StopAccept' and `Join'. Connections that has no data
    // transmission for `idle_timeout_sec' will be closed automatically iff
    // `idle_timeout_sec' > 0
    // Return 0 on success, -1 otherwise.
    int StartAccept(int listened_fd, int idle_timeout_sec,
                    const std::shared_ptr<SocketSSLContext>& ssl_ctx,
                    bool force_ssl);

    // [thread-safe] Stop accepting connections.
    // `closewait_ms' is not used anymore.
    void StopAccept(int /*closewait_ms*/);

    // Wait until all existing Sockets(defined in socket.h) are recycled.
    void Join();

    // The parameter to StartAccept. Negative when acceptor is stopped.
    int listened_fd() const { return _listened_fd; }

    // Get number of existing connections.
    size_t ConnectionCount() const;

    // Clear `conn_list' and append all connections into it.
    void ListConnections(std::vector<SocketId>* conn_list);

    // Clear `conn_list' and append all most `max_copied' connections into it.
    void ListConnections(std::vector<SocketId>* conn_list, size_t max_copied);

    Status status() const { return _status; }

private:
    // Accept connections.
    static void OnNewConnectionsUntilEAGAIN(Socket* m);
    static void OnNewConnections(Socket* m);

    static void* CloseIdleConnections(void* arg);
    
    // Initialize internal structure. 
    int Initialize();

    // Remove the accepted socket `sock' from inside
    void BeforeRecycle(Socket* sock) override;

    fiber_keytable_pool_t* _keytable_pool; // owned by Server
    Status _status;
    int _idle_timeout_sec;
    fiber_t _close_idle_tid;

    int _listened_fd;
    // The Socket tso accept connections.
    SocketId _acception_id;

    mutil::Mutex _map_mutex;
    mutil::ConditionVariable _empty_cond;
    
    // The map containing all the accepted sockets
    SocketMap _socket_map;

    bool _force_ssl;
    std::shared_ptr<SocketSSLContext> _ssl_ctx;

    // Whether to use rdma or not
    bool _use_rdma;

    // Acceptor belongs to this tag
    fiber_tag_t _fiber_tag;
};

} // namespace melon
