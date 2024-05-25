// Copyright 2023 The Elastic-AI Authors.
// part of Elastic AI Search
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//

#include <melon/raft/node.h>
#include <melon/raft/node_manager.h>
#include <melon/raft/file_service.h>
#include <melon/raft/builtin_service_impl.h>
#include <melon/raft/cli_service.h>

namespace melon::raft {

    NodeManager::NodeManager() {}

    NodeManager::~NodeManager() {}

    bool NodeManager::server_exists(mutil::EndPoint addr) {
        MELON_SCOPED_LOCK(_mutex);
        if (addr.ip != mutil::IP_ANY) {
            mutil::EndPoint any_addr(mutil::IP_ANY, addr.port);
            if (_addr_set.find(any_addr) != _addr_set.end()) {
                return true;
            }
        }
        return _addr_set.find(addr) != _addr_set.end();
    }

    void NodeManager::remove_address(mutil::EndPoint addr) {
        MELON_SCOPED_LOCK(_mutex);
        _addr_set.erase(addr);
    }

    int NodeManager::add_service(melon::Server *server,
                                 const mutil::EndPoint &listen_address) {
        if (server == NULL) {
            MLOG(ERROR) << "server is NULL";
            return -1;
        }
        if (server_exists(listen_address)) {
            return 0;
        }

        if (0 != server->AddService(file_service(), melon::SERVER_DOESNT_OWN_SERVICE)) {
            MLOG(ERROR) << "Fail to add FileService";
            return -1;
        }

        if (0 != server->AddService(
                new RaftServiceImpl(listen_address),
                melon::SERVER_OWNS_SERVICE)) {
            MLOG(ERROR) << "Fail to add RaftService";
            return -1;
        }

        if (0 != server->AddService(new RaftStatImpl, melon::SERVER_OWNS_SERVICE)) {
            MLOG(ERROR) << "Fail to add RaftStatService";
            return -1;
        }
        if (0 != server->AddService(new CliServiceImpl, melon::SERVER_OWNS_SERVICE)) {
            MLOG(ERROR) << "Fail to add CliService";
            return -1;
        }

        {
            MELON_SCOPED_LOCK(_mutex);
            _addr_set.insert(listen_address);
        }
        return 0;
    }

    size_t NodeManager::_add_node(Maps &m, const NodeImpl *node) {
        NodeId node_id = node->node_id();
        std::pair<NodeMap::iterator, bool> ret = m.node_map.insert(
                NodeMap::value_type(node_id, const_cast<NodeImpl *>(node)));
        if (ret.second) {
            m.group_map.insert(GroupMap::value_type(
                    node_id.group_id, const_cast<NodeImpl *>(node)));
            return 1;
        }
        return 0;
    }

    size_t NodeManager::_remove_node(Maps &m, const NodeImpl *node) {
        NodeMap::iterator iter = m.node_map.find(node->node_id());
        if (iter == m.node_map.end() || iter->second.get() != node) {
            // ^^
            // Avoid duplicated nodes
            return 0;
        }
        m.node_map.erase(iter);
        std::pair<GroupMap::iterator, GroupMap::iterator>
                range = m.group_map.equal_range(node->node_id().group_id);
        for (GroupMap::iterator it = range.first; it != range.second; ++it) {
            if (it->second == node) {
                m.group_map.erase(it);
                return 1;
            }
        }
        MCHECK(false) << "Can't reach here";
        return 0;
    }

    bool NodeManager::add(NodeImpl *node) {
        // check address ok?
        if (!server_exists(node->node_id().peer_id.addr)) {
            return false;
        }

        return _nodes.Modify(_add_node, node) != 0;
    }

    bool NodeManager::remove(NodeImpl *node) {
        return _nodes.Modify(_remove_node, node) != 0;
    }

    scoped_refptr<NodeImpl> NodeManager::get(const GroupId &group_id, const PeerId &peer_id) {
        mutil::DoublyBufferedData<Maps>::ScopedPtr ptr;
        if (_nodes.Read(&ptr) != 0) {
            return NULL;
        }
        NodeMap::const_iterator it = ptr->node_map.find(NodeId(group_id, peer_id));
        if (it != ptr->node_map.end()) {
            return it->second;
        }
        return NULL;
    }

    void NodeManager::get_nodes_by_group_id(
            const GroupId &group_id, std::vector<scoped_refptr<NodeImpl> > *nodes) {

        nodes->clear();
        mutil::DoublyBufferedData<Maps>::ScopedPtr ptr;
        if (_nodes.Read(&ptr) != 0) {
            return;
        }
        std::pair<GroupMap::const_iterator, GroupMap::const_iterator>
                range = ptr->group_map.equal_range(group_id);
        for (GroupMap::const_iterator it = range.first; it != range.second; ++it) {
            nodes->push_back(it->second);
        }
    }

    void NodeManager::get_all_nodes(std::vector<scoped_refptr<NodeImpl> > *nodes) {
        nodes->clear();
        mutil::DoublyBufferedData<Maps>::ScopedPtr ptr;
        if (_nodes.Read(&ptr) != 0) {
            return;
        }
        nodes->reserve(ptr->group_map.size());
        for (GroupMap::const_iterator
                     it = ptr->group_map.begin(); it != ptr->group_map.end(); ++it) {
            nodes->push_back(it->second);
        }
    }

}  //  namespace melon::raft

