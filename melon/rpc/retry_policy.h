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



#ifndef MELON_RPC_RETRY_POLICY_H_
#define MELON_RPC_RETRY_POLICY_H_

#include <melon/rpc/controller.h>


namespace melon {

// Inherit this class to customize when the RPC should be retried.
class RetryPolicy {
public:
    virtual ~RetryPolicy() = default;
    
    // Returns true if the RPC represented by `controller' should be retried.
    // [Example]
    // By default, HTTP errors are not retried, but you need to retry
    // HTTP_STATUS_FORBIDDEN in your app. You can implement the RetryPolicy
    // as follows:
    //
    //   class MyRetryPolicy : public melon::RetryPolicy {
    //   public:
    //     bool DoRetry(const melon::Controller* cntl) const {
    //       if (cntl->ErrorCode() == 0) { // don't retry successful RPC
    //         return false;
    //       }
    //       if (cntl->ErrorCode() == melon::EHTTP && // http errors
    //           cntl->http_response().status_code() == melon::HTTP_STATUS_FORBIDDEN) {
    //         return true;
    //       }
    //       // Leave other cases to default.
    //       return melon::DefaultRetryPolicy()->DoRetry(cntl);
    //     }
    //   };
    // 
    // You can retry unqualified responses even if the RPC was successful
    //   class MyRetryPolicy : public melon::RetryPolicy {
    //   public:
    //     bool DoRetry(const melon::Controller* cntl) const {
    //       if (cntl->ErrorCode() == 0) { // successful RPC
    //         if (!is_qualified(cntl->response())) {
    //           cntl->response()->Clear();  // reset the response
    //           return true;
    //         }
    //         return false;
    //       }
    //       // Leave other cases to default.
    //       return melon::DefaultRetryPolicy()->DoRetry(cntl);
    //     }
    //   };
    virtual bool DoRetry(const Controller* controller) const = 0;
    //                                                   ^
    //                                don't forget the const modifier

    // Returns the backoff time in milliseconds before every retry.
    virtual int32_t GetBackoffTimeMs(const Controller* controller) const { return 0; }
    //                                                               ^
    //                                             don't forget the const modifier

    // Returns true if enable retry backoff in pthread, otherwise returns false.
    virtual bool CanRetryBackoffInPthread() const { return false; }
    //                                        ^
    //                     don't forget the const modifier
};

// Get the RetryPolicy used by melon.
const RetryPolicy* DefaultRetryPolicy();

class RpcRetryPolicy : public RetryPolicy {
public:
    bool DoRetry(const Controller* controller) const override;
};

class RpcRetryPolicyWithFixedBackoff : public RpcRetryPolicy {
public:
    RpcRetryPolicyWithFixedBackoff(int32_t backoff_time_ms,
                                   int32_t no_backoff_remaining_rpc_time_ms,
                                   bool retry_backoff_in_pthread)
    : _backoff_time_ms(backoff_time_ms)
    , _no_backoff_remaining_rpc_time_ms(no_backoff_remaining_rpc_time_ms)
    , _retry_backoff_in_pthread(retry_backoff_in_pthread) {}

    int32_t GetBackoffTimeMs(const Controller* controller) const override;

    bool CanRetryBackoffInPthread() const override { return _retry_backoff_in_pthread; }


private:
    int32_t _backoff_time_ms;
    // If remaining rpc time is less than `_no_backoff_remaining_rpc_time', no backoff.
    int32_t _no_backoff_remaining_rpc_time_ms;
    bool _retry_backoff_in_pthread;
};

class RpcRetryPolicyWithJitteredBackoff : public RpcRetryPolicy {
public:
    RpcRetryPolicyWithJitteredBackoff(int32_t min_backoff_time_ms,
                                      int32_t max_backoff_time_ms,
                                      int32_t no_backoff_remaining_rpc_time_ms,
                                      bool retry_backoff_in_pthread)
            : _min_backoff_time_ms(min_backoff_time_ms)
            , _max_backoff_time_ms(max_backoff_time_ms)
            , _no_backoff_remaining_rpc_time_ms(no_backoff_remaining_rpc_time_ms)
            , _retry_backoff_in_pthread(retry_backoff_in_pthread) {}

    int32_t GetBackoffTimeMs(const Controller* controller) const override;

    bool CanRetryBackoffInPthread() const override { return _retry_backoff_in_pthread; }

private:
    // Generate jittered backoff time between [_min_backoff_ms, _max_backoff_ms].
    int32_t _min_backoff_time_ms;
    int32_t _max_backoff_time_ms;
    // If remaining rpc time is less than `_no_backoff_remaining_rpc_time', no backoff.
    int32_t _no_backoff_remaining_rpc_time_ms;
    bool _retry_backoff_in_pthread;
};

} // namespace melon


#endif  // MELON_RPC_RETRY_POLICY_H_
