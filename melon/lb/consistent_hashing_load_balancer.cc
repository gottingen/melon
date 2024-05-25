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


#include <algorithm>                                           // std::set_union
#include <array>
#include <gflags/gflags.h>
#include <melon/utility/containers/flat_map.h>
#include <melon/utility/errno.h>
#include <melon/utility/strings/string_number_conversions.h>
#include <melon/rpc/socket.h>
#include <melon/lb/consistent_hashing_load_balancer.h>
#include <melon/rpc/policy/hasher.h>

namespace melon::policy {
    // Defined in hasher.cpp.
    const char *GetHashName(melon::policy::HashFunc hasher);
}  // namespace melon::policy
namespace melon::lb {

    // TODO: or 160?
    DEFINE_int32(chash_num_replicas, 100,
                 "default number of replicas per server in chash");


    class ReplicaPolicy {
    public:
        virtual ~ReplicaPolicy() = default;

        virtual bool Build(ServerId server,
                           size_t num_replicas,
                           std::vector<ConsistentHashingLoadBalancer::Node> *replicas) const = 0;

        virtual const char *name() const = 0;
    };

    class DefaultReplicaPolicy : public ReplicaPolicy {
    public:
        DefaultReplicaPolicy(melon::policy::HashFunc hash) : _hash_func(hash) {}

        virtual bool Build(ServerId server,
                           size_t num_replicas,
                           std::vector<ConsistentHashingLoadBalancer::Node> *replicas) const;

        virtual const char *name() const { return melon::policy::GetHashName(_hash_func); }

    private:
        melon::policy::HashFunc _hash_func;
    };

    bool DefaultReplicaPolicy::Build(ServerId server,
                                     size_t num_replicas,
                                     std::vector<ConsistentHashingLoadBalancer::Node> *replicas) const {
        SocketUniquePtr ptr;
        if (Socket::AddressFailedAsWell(server.id, &ptr) == -1) {
            return false;
        }
        replicas->clear();
        for (size_t i = 0; i < num_replicas; ++i) {
            char host[256];
            int len = snprintf(host, sizeof(host), "%s-%lu",
                               endpoint2str(ptr->remote_side()).c_str(), i);
            ConsistentHashingLoadBalancer::Node node;
            node.hash = _hash_func(host, len);
            node.server_sock = server;
            node.server_addr = ptr->remote_side();
            replicas->push_back(node);
        }
        return true;
    }

    class KetamaReplicaPolicy : public ReplicaPolicy {
    public:
        virtual bool Build(ServerId server,
                           size_t num_replicas,
                           std::vector<ConsistentHashingLoadBalancer::Node> *replicas) const;

        virtual const char *name() const { return "ketama"; }
    };

    bool KetamaReplicaPolicy::Build(ServerId server,
                                    size_t num_replicas,
                                    std::vector<ConsistentHashingLoadBalancer::Node> *replicas) const {
        SocketUniquePtr ptr;
        if (Socket::AddressFailedAsWell(server.id, &ptr) == -1) {
            return false;
        }
        replicas->clear();
        const size_t points_per_hash = 4;
        MCHECK(num_replicas % points_per_hash == 0)
        << "Ketam hash replicas number(" << num_replicas << ") should be n*4";
        for (size_t i = 0; i < num_replicas / points_per_hash; ++i) {
            char host[32];
            int len = snprintf(host, sizeof(host), "%s-%lu",
                               endpoint2str(ptr->remote_side()).c_str(), i);
            unsigned char digest[16];
            melon::policy::MD5HashSignature(host, len, digest);
            for (size_t j = 0; j < points_per_hash; ++j) {
                ConsistentHashingLoadBalancer::Node node;
                node.server_sock = server;
                node.server_addr = ptr->remote_side();
                node.hash = ((uint32_t) (digest[3 + j * 4] & 0xFF) << 24)
                            | ((uint32_t) (digest[2 + j * 4] & 0xFF) << 16)
                            | ((uint32_t) (digest[1 + j * 4] & 0xFF) << 8)
                            | (digest[0 + j * 4] & 0xFF);
                replicas->push_back(node);
            }
        }
        return true;
    }

    namespace {

        pthread_once_t s_replica_policy_once = PTHREAD_ONCE_INIT;
        const std::array<const ReplicaPolicy *, CONS_HASH_LB_LAST> *g_replica_policy = nullptr;

        void InitReplicaPolicy() {
            g_replica_policy = new std::array<const ReplicaPolicy *, CONS_HASH_LB_LAST>({
                                                                                                new DefaultReplicaPolicy(
                                                                                                        melon::policy::MurmurHash32),
                                                                                                new DefaultReplicaPolicy(
                                                                                                        melon::policy::MD5Hash32),
                                                                                                new KetamaReplicaPolicy
                                                                                        });
        }

        inline const ReplicaPolicy *GetReplicaPolicy(ConsistentHashingLoadBalancerType type) {
            pthread_once(&s_replica_policy_once, InitReplicaPolicy);
            return g_replica_policy->at(type);
        }

    } // namespace

    ConsistentHashingLoadBalancer::ConsistentHashingLoadBalancer(
            ConsistentHashingLoadBalancerType type)
            : _num_replicas(FLAGS_chash_num_replicas), _type(type) {
        MCHECK(GetReplicaPolicy(_type))
        << "Fail to find replica policy for consistency lb type: '" << _type << '\'';
    }

    size_t ConsistentHashingLoadBalancer::AddBatch(
            std::vector<Node> &bg, const std::vector<Node> &fg,
            const std::vector<Node> &servers, bool *executed) {
        if (*executed) {
            // Hack DBD
            return fg.size() - bg.size();
        }
        *executed = true;
        bg.resize(fg.size() + servers.size());
        bg.resize(std::set_union(fg.begin(), fg.end(),
                                 servers.begin(), servers.end(), bg.begin())
                  - bg.begin());
        return bg.size() - fg.size();
    }

    size_t ConsistentHashingLoadBalancer::RemoveBatch(
            std::vector<Node> &bg, const std::vector<Node> &fg,
            const std::vector<ServerId> &servers, bool *executed) {
        if (*executed) {
            return bg.size() - fg.size();
        }
        *executed = true;
        if (servers.empty()) {
            bg = fg;
            return 0;
        }
        mutil::FlatSet<ServerId> id_set;
        bool use_set = true;
        if (id_set.init(servers.size() * 2) == 0) {
            for (size_t i = 0; i < servers.size(); ++i) {
                if (id_set.insert(servers[i]) == NULL) {
                    use_set = false;
                    break;
                }
            }
        } else {
            use_set = false;
        }
        MCHECK(use_set) << "Fail to construct id_set, " << berror();
        bg.clear();
        for (size_t i = 0; i < fg.size(); ++i) {
            const bool removed =
                    use_set ? (id_set.seek(fg[i].server_sock) != NULL)
                            : (std::find(servers.begin(), servers.end(),
                                         fg[i].server_sock) != servers.end());
            if (!removed) {
                bg.push_back(fg[i]);
            }
        }
        return fg.size() - bg.size();
    }

    size_t ConsistentHashingLoadBalancer::Remove(
            std::vector<Node> &bg, const std::vector<Node> &fg,
            const ServerId &server, bool *executed) {
        if (*executed) {
            return bg.size() - fg.size();
        }
        *executed = true;
        bg.clear();
        for (size_t i = 0; i < fg.size(); ++i) {
            if (fg[i].server_sock != server) {
                bg.push_back(fg[i]);
            }
        }
        return fg.size() - bg.size();
    }

    bool ConsistentHashingLoadBalancer::AddServer(const ServerId &server) {
        std::vector<Node> add_nodes;
        add_nodes.reserve(_num_replicas);
        if (!GetReplicaPolicy(_type)->Build(server, _num_replicas, &add_nodes)) {
            return false;
        }
        std::sort(add_nodes.begin(), add_nodes.end());
        bool executed = false;
        const size_t ret = _db_hash_ring.ModifyWithForeground(
                AddBatch, add_nodes, &executed);
        MCHECK(ret == 0 || ret == _num_replicas) << ret;
        return ret != 0;
    }

    size_t ConsistentHashingLoadBalancer::AddServersInBatch(
            const std::vector<ServerId> &servers) {
        std::vector<Node> add_nodes;
        add_nodes.reserve(servers.size() * _num_replicas);
        std::vector<Node> replicas;
        replicas.reserve(_num_replicas);
        for (size_t i = 0; i < servers.size(); ++i) {
            replicas.clear();
            if (GetReplicaPolicy(_type)->Build(servers[i], _num_replicas, &replicas)) {
                add_nodes.insert(add_nodes.end(), replicas.begin(), replicas.end());
            }
        }
        std::sort(add_nodes.begin(), add_nodes.end());
        bool executed = false;
        const size_t ret = _db_hash_ring.ModifyWithForeground(AddBatch, add_nodes, &executed);
        MCHECK(ret % _num_replicas == 0);
        const size_t n = ret / _num_replicas;
        MLOG_IF(ERROR, n != servers.size())
        << "Fail to AddServersInBatch, expected " << servers.size()
        << " actually " << n;
        return n;
    }

    bool ConsistentHashingLoadBalancer::RemoveServer(const ServerId &server) {
        bool executed = false;
        const size_t ret = _db_hash_ring.ModifyWithForeground(Remove, server, &executed);
        MCHECK(ret == 0 || ret == _num_replicas);
        return ret != 0;
    }

    size_t ConsistentHashingLoadBalancer::RemoveServersInBatch(
            const std::vector<ServerId> &servers) {
        bool executed = false;
        const size_t ret = _db_hash_ring.ModifyWithForeground(RemoveBatch, servers, &executed);
        MCHECK(ret % _num_replicas == 0);
        const size_t n = ret / _num_replicas;
        MLOG_IF(ERROR, n != servers.size())
        << "Fail to RemoveServersInBatch, expected " << servers.size()
        << " actually " << n;
        return n;
    }

    LoadBalancer *ConsistentHashingLoadBalancer::New(const mutil::StringPiece &params) const {
        ConsistentHashingLoadBalancer *lb =
                new(std::nothrow) ConsistentHashingLoadBalancer(_type);
        if (lb && !lb->SetParameters(params)) {
            delete lb;
            lb = nullptr;
        }
        return lb;
    }

    void ConsistentHashingLoadBalancer::Destroy() {
        delete this;
    }

    int ConsistentHashingLoadBalancer::SelectServer(
            const SelectIn &in, SelectOut *out) {
        if (!in.has_request_code) {
            MLOG(ERROR) << "Controller.set_request_code() is required";
            return EINVAL;
        }
        if (in.request_code > UINT_MAX) {
            MLOG(ERROR) << "request_code must be 32-bit currently";
            return EINVAL;
        }
        mutil::DoublyBufferedData<std::vector<Node> >::ScopedPtr s;
        if (_db_hash_ring.Read(&s) != 0) {
            return ENOMEM;
        }
        if (s->empty()) {
            return ENODATA;
        }
        std::vector<Node>::const_iterator choice =
                std::lower_bound(s->begin(), s->end(), (uint32_t) in.request_code);
        if (choice == s->end()) {
            choice = s->begin();
        }
        for (size_t i = 0; i < s->size(); ++i) {
            if (((i + 1) == s->size() // always take last chance
                 || !ExcludedServers::IsExcluded(in.excluded, choice->server_sock.id))
                && Socket::Address(choice->server_sock.id, out->ptr) == 0
                && (*out->ptr)->IsAvailable()) {
                return 0;
            } else {
                if (++choice == s->end()) {
                    choice = s->begin();
                }
            }
        }
        return EHOSTDOWN;
    }

    void ConsistentHashingLoadBalancer::Describe(
            std::ostream &os, const DescribeOptions &options) {
        if (!options.verbose) {
            os << "c_hash";
            return;
        }
        os << "ConsistentHashingLoadBalancer {\n"
           << "  hash function: " << GetReplicaPolicy(_type)->name() << '\n'
           << "  replica per host: " << _num_replicas << '\n';
        std::map<mutil::EndPoint, double> load_map;
        GetLoads(&load_map);
        os << "  number of hosts: " << load_map.size() << '\n';
        os << "  load of hosts: {\n";
        double expected_load_per_server = 1.0 / load_map.size();
        double load_sum = 0;
        double load_sqr_sum = 0;
        for (std::map<mutil::EndPoint, double>::iterator
                     it = load_map.begin(); it != load_map.end(); ++it) {
            os << "    " << it->first << ": " << it->second << '\n';
            double normalized_load = it->second / expected_load_per_server;
            load_sum += normalized_load;
            load_sqr_sum += normalized_load * normalized_load;
        }
        os << "  }\n";
        os << "deviation: "
           << sqrt(load_sqr_sum * load_map.size() - load_sum * load_sum)
              / load_map.size();
        os << "}\n";
    }

    void ConsistentHashingLoadBalancer::GetLoads(
            std::map<mutil::EndPoint, double> *load_map) {
        load_map->clear();
        std::map<mutil::EndPoint, uint32_t> count_map;
        do {
            mutil::DoublyBufferedData<std::vector<Node> >::ScopedPtr s;
            if (_db_hash_ring.Read(&s) != 0) {
                break;
            }
            if (s->empty()) {
                break;
            }
            count_map[s->begin()->server_addr] +=
                    s->begin()->hash + (UINT_MAX - (s->end() - 1)->hash);
            for (size_t i = 1; i < s->size(); ++i) {
                count_map[(*s.get())[i].server_addr] +=
                        (*s.get())[i].hash - (*s.get())[i - 1].hash;
            }
        } while (0);
        for (std::map<mutil::EndPoint, uint32_t>::iterator
                     it = count_map.begin(); it != count_map.end(); ++it) {
            (*load_map)[it->first] = (double) it->second / UINT_MAX;
        }
    }

    bool ConsistentHashingLoadBalancer::SetParameters(const mutil::StringPiece &params) {
        for (mutil::KeyValuePairsSplitter sp(params.begin(), params.end(), ' ', '=');
             sp; ++sp) {
            if (sp.value().empty()) {
                MLOG(ERROR) << "Empty value for " << sp.key() << " in lb parameter";
                return false;
            }
            if (sp.key() == "replicas") {
                if (!mutil::StringToSizeT(sp.value(), &_num_replicas)) {
                    return false;
                }
                continue;
            }
            MLOG(ERROR) << "Failed to set this unknown parameters " << sp.key_and_value();
        }
        return true;
    }

} // namespace melon::lb
