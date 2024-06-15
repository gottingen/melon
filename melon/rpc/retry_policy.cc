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



#include <melon/rpc/retry_policy.h>
#include <melon/base/fast_rand.h>


namespace melon {

bool RpcRetryPolicy::DoRetry(const Controller* controller) const {
    const int error_code = controller->ErrorCode();
    if (!error_code) {
        return false;
    }
    return (EFAILEDSOCKET == error_code
            || EEOF == error_code
            || EHOSTDOWN == error_code
            || ELOGOFF == error_code
            || ETIMEDOUT == error_code // This is not timeout of RPC.
            || ELIMIT == error_code
            || ENOENT == error_code
            || EPIPE == error_code
            || ECONNREFUSED == error_code
            || ECONNRESET == error_code
            || ENODATA == error_code
            || EOVERCROWDED == error_code
            || EH2RUNOUTSTREAMS == error_code);
}

// NOTE(gejun): g_default_policy can't be deleted on process's exit because
// client-side may still retry and use the policy at exit
static pthread_once_t g_default_policy_once = PTHREAD_ONCE_INIT;
static RpcRetryPolicy* g_default_policy = NULL;
static void init_default_policy() {
    g_default_policy = new RpcRetryPolicy;
}
const RetryPolicy* DefaultRetryPolicy() {
    pthread_once(&g_default_policy_once, init_default_policy);
    return g_default_policy;
}

int32_t RpcRetryPolicyWithFixedBackoff::GetBackoffTimeMs(
    const Controller* controller) const {
    int64_t remaining_rpc_time_ms =
        (controller->deadline_us() - mutil::gettimeofday_us()) / 1000;
    if (remaining_rpc_time_ms < _no_backoff_remaining_rpc_time_ms) {
        return 0;
    }
    return _backoff_time_ms;
}

int32_t RpcRetryPolicyWithJitteredBackoff::GetBackoffTimeMs(
    const Controller* controller) const {
    int64_t remaining_rpc_time_ms =
        (controller->deadline_us() - mutil::gettimeofday_us()) / 1000;
    if (remaining_rpc_time_ms < _no_backoff_remaining_rpc_time_ms) {
        return 0;
    }
    return mutil::fast_rand_in(_min_backoff_time_ms,
                               _max_backoff_time_ms);
}

} // namespace melon
