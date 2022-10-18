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


#include <vector>
#include <gflags/gflags.h>
#include "melon/rpc/cluster_recover_policy.h"
#include "melon/base/scoped_lock.h"
#include "melon/rpc/server_id.h"
#include "melon/rpc/socket.h"
#include "melon/base/fast_rand.h"
#include "melon/times/time.h"
#include "melon/strings/string_splitter.h"
#include "melon/strings/numbers.h"

namespace melon::rpc {

    DEFINE_int64(detect_available_server_interval_ms, 10, "The interval "
                                                          "to detect available server count in DefaultClusterRecoverPolicy");

    DefaultClusterRecoverPolicy::DefaultClusterRecoverPolicy(
            int64_t min_working_instances, int64_t hold_seconds)
            : _recovering(false), _min_working_instances(min_working_instances), _last_usable(0),
              _last_usable_change_time_ms(0), _hold_seconds(hold_seconds), _usable_cache(0), _usable_cache_time_ms(0) {}

    void DefaultClusterRecoverPolicy::StartRecover() {
        std::unique_lock<std::mutex> mu(_mutex);
        _recovering = true;
    }

    bool DefaultClusterRecoverPolicy::StopRecoverIfNecessary() {
        if (!_recovering) {
            return false;
        }
        int64_t now_ms = melon::time_now().to_unix_millis();
        std::unique_lock<std::mutex> mu(_mutex);
        if (_last_usable_change_time_ms != 0 && _last_usable != 0 &&
            (now_ms - _last_usable_change_time_ms > _hold_seconds * 1000)) {
            _recovering = false;
            _last_usable = 0;
            _last_usable_change_time_ms = 0;
            mu.unlock();
            return false;
        }
        mu.unlock();
        return true;
    }

    uint64_t DefaultClusterRecoverPolicy::GetUsableServerCount(
            int64_t now_ms, const std::vector<ServerId> &server_list) {
        if (now_ms - _usable_cache_time_ms < FLAGS_detect_available_server_interval_ms) {
            return _usable_cache;
        }
        uint64_t usable = 0;
        size_t n = server_list.size();
        SocketUniquePtr ptr;
        for (size_t i = 0; i < n; ++i) {
            if (Socket::Address(server_list[i].id, &ptr) == 0
                && ptr->IsAvailable()) {
                usable++;
            }
        }
        {
            std::unique_lock<std::mutex> mu(_mutex);
            _usable_cache = usable;
            _usable_cache_time_ms = now_ms;
        }
        return _usable_cache;
    }


    bool DefaultClusterRecoverPolicy::DoReject(const std::vector<ServerId> &server_list) {
        if (!_recovering) {
            return false;
        }
        int64_t now_ms = melon::time_now().to_unix_millis();
        uint64_t usable = GetUsableServerCount(now_ms, server_list);
        if (_last_usable != usable) {
            std::unique_lock<std::mutex> mu(_mutex);
            if (_last_usable != usable) {
                _last_usable = usable;
                _last_usable_change_time_ms = now_ms;
            }
        }
        if (melon::base::fast_rand_less_than(_min_working_instances) >= usable) {
            return true;
        }
        return false;
    }

    bool GetRecoverPolicyByParams(const std::string_view &params,
                                  std::shared_ptr<ClusterRecoverPolicy> *ptr_out) {
        int64_t min_working_instances = -1;
        int64_t hold_seconds = -1;
        bool has_meet_params = false;
        for (melon::KeyValuePairsSplitter sp(params.begin(), params.end(), ' ', '=');
             sp; ++sp) {
            if (sp.value().empty()) {
                MELON_LOG(ERROR) << "Empty value for " << sp.key() << " in lb parameter";
                return false;
            }
            if (sp.key() == "min_working_instances") {
                int64_t r;
                if (!melon::simple_atoi(sp.value(), &r)) {
                    return false;
                }

                min_working_instances = r;
                has_meet_params = true;
                continue;
            } else if (sp.key() == "hold_seconds") {
                int64_t r;
                if (!melon::simple_atoi(sp.value(), &r)) {
                    return false;
                }
                hold_seconds = r;
                has_meet_params = true;
                continue;
            }
            MELON_LOG(ERROR) << "Failed to set this unknown parameters " << sp.key_and_value();
            return false;
        }
        if (min_working_instances > 0 && hold_seconds > 0) {
            ptr_out->reset(
                    new DefaultClusterRecoverPolicy(min_working_instances, hold_seconds));
        } else if (has_meet_params) {
            // In this case, user set some params but not in the right way, just return
            // false to let user take care of this situation.
            MELON_LOG(ERROR) << "Invalid params=`" << params << "'";
            return false;
        }
        return true;
    }

} // namespace melon::rpc
