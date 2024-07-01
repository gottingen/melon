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



#include <vector>
#include <turbo/flags/flag.h>
#include <melon/rpc/cluster_recover_policy.h>
#include <melon/utility/scoped_lock.h>
#include <melon/utility/synchronization/lock.h>
#include <melon/rpc/server_id.h>
#include <melon/rpc/socket.h>
#include <melon/utility/fast_rand.h>
#include <melon/utility/time.h>
#include <melon/utility/string_splitter.h>

TURBO_FLAG(int64_t, detect_available_server_interval_ms, 10, "The interval "
                                                             "to detect available server count in DefaultClusterRecoverPolicy");
namespace melon {


    DefaultClusterRecoverPolicy::DefaultClusterRecoverPolicy(
            int64_t min_working_instances, int64_t hold_seconds)
            : _recovering(false), _min_working_instances(min_working_instances), _last_usable(0),
              _last_usable_change_time_ms(0), _hold_seconds(hold_seconds), _usable_cache(0), _usable_cache_time_ms(0) {}

    void DefaultClusterRecoverPolicy::StartRecover() {
        std::unique_lock<mutil::Mutex> mu(_mutex);
        _recovering = true;
    }

    bool DefaultClusterRecoverPolicy::StopRecoverIfNecessary() {
        if (!_recovering) {
            return false;
        }
        int64_t now_ms = mutil::gettimeofday_ms();
        std::unique_lock<mutil::Mutex> mu(_mutex);
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
        if (now_ms - _usable_cache_time_ms < turbo::get_flag(FLAGS_detect_available_server_interval_ms)) {
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
            std::unique_lock<mutil::Mutex> mu(_mutex);
            _usable_cache = usable;
            _usable_cache_time_ms = now_ms;
        }
        return _usable_cache;
    }


    bool DefaultClusterRecoverPolicy::DoReject(const std::vector<ServerId> &server_list) {
        if (!_recovering) {
            return false;
        }
        int64_t now_ms = mutil::gettimeofday_ms();
        uint64_t usable = GetUsableServerCount(now_ms, server_list);
        if (_last_usable != usable) {
            std::unique_lock<mutil::Mutex> mu(_mutex);
            if (_last_usable != usable) {
                _last_usable = usable;
                _last_usable_change_time_ms = now_ms;
            }
        }
        if (mutil::fast_rand_less_than(_min_working_instances) >= usable) {
            return true;
        }
        return false;
    }

    bool GetRecoverPolicyByParams(const std::string_view &params,
                                  std::shared_ptr<ClusterRecoverPolicy> *ptr_out) {
        int64_t min_working_instances = -1;
        int64_t hold_seconds = -1;
        bool has_meet_params = false;
        for (mutil::KeyValuePairsSplitter sp(params.begin(), params.end(), ' ', '=');
             sp; ++sp) {
            if (sp.value().empty()) {
                LOG(ERROR) << "Empty value for " << sp.key() << " in lb parameter";
                return false;
            }
            if (sp.key() == "min_working_instances") {
                if (!mutil::StringToInt64(sp.value(), &min_working_instances)) {
                    return false;
                }
                has_meet_params = true;
                continue;
            } else if (sp.key() == "hold_seconds") {
                if (!mutil::StringToInt64(sp.value(), &hold_seconds)) {
                    return false;
                }
                has_meet_params = true;
                continue;
            }
            LOG(ERROR) << "Failed to set this unknown parameters " << sp.key_and_value();
            return false;
        }
        if (min_working_instances > 0 && hold_seconds > 0) {
            ptr_out->reset(
                    new DefaultClusterRecoverPolicy(min_working_instances, hold_seconds));
        } else if (has_meet_params) {
            // In this case, user set some params but not in the right way, just return
            // false to let user take care of this situation.
            LOG(ERROR) << "Invalid params=`" << params << "'";
            return false;
        }
        return true;
    }

} // namespace melon
