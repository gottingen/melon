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


#include <melon/utility/macros.h>
#include <melon/base/fast_rand.h>
#include <melon/rpc/socket.h>
#include <melon/lb/randomized_load_balancer.h>
#include <melon/utility/strings/string_number_conversions.h>

namespace melon::lb {

    const uint32_t prime_offset[] = {
#include "melon/fiber/offset_inl.list"
    };

    inline uint32_t GenRandomStride() {
        return prime_offset[mutil::fast_rand_less_than(ARRAY_SIZE(prime_offset))];
    }

    bool RandomizedLoadBalancer::Add(Servers &bg, const ServerId &id) {
        if (bg.server_list.capacity() < 128) {
            bg.server_list.reserve(128);
        }
        std::map<ServerId, size_t>::iterator it = bg.server_map.find(id);
        if (it != bg.server_map.end()) {
            return false;
        }
        bg.server_map[id] = bg.server_list.size();
        bg.server_list.push_back(id);
        return true;
    }

    bool RandomizedLoadBalancer::Remove(Servers &bg, const ServerId &id) {
        std::map<ServerId, size_t>::iterator it = bg.server_map.find(id);
        if (it != bg.server_map.end()) {
            size_t index = it->second;
            bg.server_list[index] = bg.server_list.back();
            bg.server_map[bg.server_list[index]] = index;
            bg.server_list.pop_back();
            bg.server_map.erase(it);
            return true;
        }
        return false;
    }

    size_t RandomizedLoadBalancer::BatchAdd(
            Servers &bg, const std::vector<ServerId> &servers) {
        size_t count = 0;
        for (size_t i = 0; i < servers.size(); ++i) {
            count += !!Add(bg, servers[i]);
        }
        return count;
    }

    size_t RandomizedLoadBalancer::BatchRemove(
            Servers &bg, const std::vector<ServerId> &servers) {
        size_t count = 0;
        for (size_t i = 0; i < servers.size(); ++i) {
            count += !!Remove(bg, servers[i]);
        }
        return count;
    }

    bool RandomizedLoadBalancer::AddServer(const ServerId &id) {
        return _db_servers.Modify(Add, id);
    }

    bool RandomizedLoadBalancer::RemoveServer(const ServerId &id) {
        return _db_servers.Modify(Remove, id);
    }

    size_t RandomizedLoadBalancer::AddServersInBatch(
            const std::vector<ServerId> &servers) {
        const size_t n = _db_servers.Modify(BatchAdd, servers);
        LOG_IF(ERROR, n != servers.size())
        << "Fail to AddServersInBatch, expected " << servers.size()
        << " actually " << n;
        return n;
    }

    size_t RandomizedLoadBalancer::RemoveServersInBatch(
            const std::vector<ServerId> &servers) {
        const size_t n = _db_servers.Modify(BatchRemove, servers);
        LOG_IF(ERROR, n != servers.size())
        << "Fail to RemoveServersInBatch, expected " << servers.size()
        << " actually " << n;
        return n;
    }

    int RandomizedLoadBalancer::SelectServer(const SelectIn &in, SelectOut *out) {
        mutil::DoublyBufferedData<Servers>::ScopedPtr s;
        if (_db_servers.Read(&s) != 0) {
            return ENOMEM;
        }
        size_t n = s->server_list.size();
        if (n == 0) {
            return ENODATA;
        }
        if (_cluster_recover_policy && _cluster_recover_policy->StopRecoverIfNecessary()) {
            if (_cluster_recover_policy->DoReject(s->server_list)) {
                return EREJECT;
            }
        }
        uint32_t stride = 0;
        size_t offset = mutil::fast_rand_less_than(n);
        for (size_t i = 0; i < n; ++i) {
            const SocketId id = s->server_list[offset].id;
            if (((i + 1) == n  // always take last chance
                 || !ExcludedServers::IsExcluded(in.excluded, id))
                && Socket::Address(id, out->ptr) == 0
                && (*out->ptr)->IsAvailable()) {
                // We found an available server
                return 0;
            }
            if (stride == 0) {
                stride = GenRandomStride();
            }
            // If `Address' failed, use `offset+stride' to retry so that
            // this failed server won't be visited again inside for
            offset = (offset + stride) % n;
        }
        if (_cluster_recover_policy) {
            _cluster_recover_policy->StartRecover();
        }
        // After we traversed the whole server list, there is still no
        // available server
        return EHOSTDOWN;
    }

    RandomizedLoadBalancer *RandomizedLoadBalancer::New(
            const mutil::StringPiece &params) const {
        RandomizedLoadBalancer *lb = new(std::nothrow) RandomizedLoadBalancer;
        if (lb && !lb->SetParameters(params)) {
            delete lb;
            lb = NULL;
        }
        return lb;
    }

    void RandomizedLoadBalancer::Destroy() {
        delete this;
    }

    void RandomizedLoadBalancer::Describe(
            std::ostream &os, const DescribeOptions &options) {
        if (!options.verbose) {
            os << "random";
            return;
        }
        os << "Randomized{";
        mutil::DoublyBufferedData<Servers>::ScopedPtr s;
        if (_db_servers.Read(&s) != 0) {
            os << "fail to read _db_servers";
        } else {
            os << "n=" << s->server_list.size() << ':';
            for (size_t i = 0; i < s->server_list.size(); ++i) {
                os << ' ' << s->server_list[i];
            }
        }
        os << '}';
    }

    bool RandomizedLoadBalancer::SetParameters(const mutil::StringPiece &params) {
        return GetRecoverPolicyByParams(params, &_cluster_recover_policy);
    }

} // namespace melon::lb
