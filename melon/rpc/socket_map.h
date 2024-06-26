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



#ifndef MELON_RPC_SOCKET_MAP_H_
#define MELON_RPC_SOCKET_MAP_H_

#include <vector>                             // std::vector
#include <melon/var/var.h>                        // melon::var::PassiveStatus
#include <melon/utility/containers/flat_map.h>        // FlatMap
#include <melon/rpc/socket_id.h>                   // SockdetId
#include <melon/proto/rpc/options.pb.h>                  // ProtocolType
#include <melon/rpc/input_messenger.h>             // InputMessageHandler
#include <melon/rpc/server_node.h>                 // ServerNode

namespace melon {

// Different signature means that the Channel needs separate sockets.
struct ChannelSignature {
    uint64_t data[2];
    
    ChannelSignature() { Reset(); }
    void Reset() { data[0] = data[1] = 0; }
};

inline bool operator==(const ChannelSignature& s1, const ChannelSignature& s2) {
    return s1.data[0] == s2.data[0] && s1.data[1] == s2.data[1];
}
inline bool operator!=(const ChannelSignature& s1, const ChannelSignature& s2) {
    return !(s1 == s2);
}

// The following fields uniquely define a Socket. In other word,
// Socket can't be shared between 2 different SocketMapKeys
struct SocketMapKey {
    explicit SocketMapKey(const mutil::EndPoint& pt)
        : peer(pt)
    {}
    SocketMapKey(const mutil::EndPoint& pt, const ChannelSignature& cs)
        : peer(pt), channel_signature(cs)
    {}
    SocketMapKey(const ServerNode& sn, const ChannelSignature& cs)
        : peer(sn), channel_signature(cs)
    {}

    ServerNode peer;
    ChannelSignature channel_signature;
};

inline bool operator==(const SocketMapKey& k1, const SocketMapKey& k2) {
    return k1.peer == k2.peer && k1.channel_signature == k2.channel_signature;
};

struct SocketMapKeyHasher {
    size_t operator()(const SocketMapKey& key) const {
        size_t h = mutil::DefaultHasher<mutil::EndPoint>()(key.peer.addr);
        h = h * 101 + mutil::DefaultHasher<std::string>()(key.peer.tag);
        h = h * 101 + key.channel_signature.data[1];
        return h;
    }
};


// Try to share the Socket to `key'. If the Socket does not exist, create one.
// The corresponding SocketId is written to `*id'. If this function returns
// successfully, SocketMapRemove() MUST be called when the Socket is not needed.
// Return 0 on success, -1 otherwise.
int SocketMapInsert(const SocketMapKey& key, SocketId* id,
                    const std::shared_ptr<SocketSSLContext>& ssl_ctx,
                    bool use_rdma);

inline int SocketMapInsert(const SocketMapKey& key, SocketId* id,
                    const std::shared_ptr<SocketSSLContext>& ssl_ctx) {
    return SocketMapInsert(key, id, ssl_ctx, false);
}

inline int SocketMapInsert(const SocketMapKey& key, SocketId* id) {
    std::shared_ptr<SocketSSLContext> empty_ptr;
    return SocketMapInsert(key, id, empty_ptr, false);
}

// Find the SocketId associated with `key'.
// Return 0 on found, -1 otherwise.
int SocketMapFind(const SocketMapKey& key, SocketId* id);

// Called once when the Socket returned by SocketMapInsert() is not needed.
void SocketMapRemove(const SocketMapKey& key);

// Put all existing Sockets into `ids'
void SocketMapList(std::vector<SocketId>* ids);

// ======================================================================
// The underlying class that can be used otherwhere for mapping endpoints
// to sockets.

// SocketMap creates sockets on-demand by calling this object.
class SocketCreator {
public:
    virtual ~SocketCreator() {}
    virtual int CreateSocket(const SocketOptions& opt, SocketId* id) = 0;
};

struct SocketMapOptions {
    // Constructed with default options.
    SocketMapOptions();
    
    // For creating sockets by need. Owned and deleted by SocketMap.
    // Default: NULL (must be set by user).
    SocketCreator* socket_creator;

    // Initial size of the map (proper size reduces number of resizes)
    // Default: 1024
    size_t suggested_map_size;
  
    // Pooled connections without data transmission for so many seconds will
    // be closed. No effect for non-positive values.
    // If idle_timeout_second_dynamic is not NULL, use the dereferenced value
    // each time instead of idle_timeout_second.
    // Default: 0 (disabled)
    const int* idle_timeout_second_dynamic;
    int idle_timeout_second;

    // Defer close of connections for so many seconds even if the connection
    // is not used by anyone. Close immediately for non-positive values.
    // If defer_close_second_dynamic is not NULL, use the dereferenced value
    // each time instead of defer_close_second.
    // Default: 0 (disabled)
    const int* defer_close_second_dynamic;
    int defer_close_second;
};

// Share sockets to the same EndPoint.
class SocketMap {
public:
    SocketMap();
    ~SocketMap();
    int Init(const SocketMapOptions&);
    int Insert(const SocketMapKey& key, SocketId* id,
               const std::shared_ptr<SocketSSLContext>& ssl_ctx,
               bool use_rdma);
    int Insert(const SocketMapKey& key, SocketId* id,
               const std::shared_ptr<SocketSSLContext>& ssl_ctx) {
        return Insert(key, id, ssl_ctx, false);   
    }
    int Insert(const SocketMapKey& key, SocketId* id) {
        std::shared_ptr<SocketSSLContext> empty_ptr;
        return Insert(key, id, empty_ptr, false);
    }

    void Remove(const SocketMapKey& key, SocketId expected_id);
    int Find(const SocketMapKey& key, SocketId* id);
    void List(std::vector<SocketId>* ids);
    void List(std::vector<mutil::EndPoint>* pts);
    const SocketMapOptions& options() const { return _options; }

private:
    void RemoveInternal(const SocketMapKey& key, SocketId id,
                        bool remove_orphan);
    void ListOrphans(int64_t defer_us, std::vector<SocketMapKey>* out);
    void WatchConnections();
    static void* RunWatchConnections(void*);
    void Print(std::ostream& os);
    static void PrintSocketMap(std::ostream& os, void* arg);
    void ShowSocketMapInVarIfNeed();

private:
    struct SingleConnection {
        int ref_count;
        Socket* socket;
        int64_t no_ref_us;
    };

    // TODO: When RpcChannels connecting to one EndPoint are frequently created
    //       and destroyed, a single map+mutex may become hot-spots.
    typedef mutil::FlatMap<SocketMapKey, SingleConnection,
                           SocketMapKeyHasher> Map;
    SocketMapOptions _options;
    mutil::Mutex _mutex;
    Map _map;
    mutil::atomic<bool> _exposed_in_var;
    melon::var::PassiveStatus<std::string>* _this_map_var;
    bool _has_close_idle_thread;
    fiber_t _close_idle_thread;
};

} // namespace melon

#endif  // MELON_RPC_SOCKET_MAP_H_
