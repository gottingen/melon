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


#ifndef  MELON_VAR_DETAIL_CALL_OP_RETURNING_VOID_H_
#define  MELON_VAR_DETAIL_CALL_OP_RETURNING_VOID_H_

namespace melon::var {
    namespace detail {

        template<typename Op, typename T1, typename T2>
        inline void call_op_returning_void(
                const Op &op, T1 &v1, const T2 &v2) {
            return op(v1, v2);
        }

    }  // namespace detail
}  // namespace melon::var

#endif  // MELON_VAR_DETAIL_CALL_OP_RETURNING_VOID_H_
