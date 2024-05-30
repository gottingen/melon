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
#include <melon/utility/memory/singleton.h>
#include <melon/utility/containers/doubly_buffered_data.h>
#include <melon/raft/raft.h>
#include <melon/raft/util.h>

namespace melon::raft {

    class NodeImpl;

    class NodeManager {
    public:
        static NodeManager *GetInstance() {
            return Singleton<NodeManager>::get();
        }

        // add raft node
        bool add(NodeImpl *node);

        // remove raft node
        bool remove(NodeImpl *node);

        // get node by group_id and peer_id
        scoped_refptr<NodeImpl> get(const GroupId &group_id, const PeerId &peer_id);

        // get all the nodes of |group_id|
        void get_nodes_by_group_id(const GroupId &group_id,
                                   std::vector<scoped_refptr<NodeImpl> > *nodes);

        void get_all_nodes(std::vector<scoped_refptr<NodeImpl> > *nodes);

        // Add service to |server| at |listen_addr|
        int add_service(melon::Server *server,
                        const mutil::EndPoint &listen_addr);

        // Return true if |addr| is reachable by a RPC Server
        bool server_exists(mutil::EndPoint addr);

        // Remove the addr from _addr_set when the backing service is destroyed
        void remove_address(mutil::EndPoint addr);

    private:
        NodeManager();

        ~NodeManager();
        DISALLOW_COPY_AND_ASSIGN(NodeManager);

        friend struct DefaultSingletonTraits<NodeManager>;

        // TODO(chenzhangyi01): replace std::map with FlatMap
        // To make implementation simplicity, we use two maps here, although
        // it works practically with only one GroupMap
        typedef std::map<NodeId, scoped_refptr<NodeImpl> > NodeMap;
        typedef std::multimap<GroupId, NodeImpl *> GroupMap;
        struct Maps {
            NodeMap node_map;
            GroupMap group_map;
        };

        // Functor to modify DBD
        static size_t _add_node(Maps &, const NodeImpl *node);

        static size_t _remove_node(Maps &, const NodeImpl *node);

        mutil::DoublyBufferedData<Maps> _nodes;

        raft_mutex_t _mutex;
        std::set<mutil::EndPoint> _addr_set;
    };

#define global_node_manager NodeManager::GetInstance()

}   //  namespace melon::raft

