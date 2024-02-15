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



#ifndef MELON_RPC_TRACEPRINTF_H_
#define MELON_RPC_TRACEPRINTF_H_

#include "melon/utility/macros.h"




namespace melon {

bool CanAnnotateSpan();
void AnnotateSpan(const char* fmt, ...);

} // namespace melon


// Use this macro to print log to /rpcz and tracing system.
// If rpcz is not enabled, arguments to this macro is NOT evaluated, don't
// have (critical) side effects in arguments.
#define TRACEPRINTF(fmt, args...)                                       \
    do {                                                                \
        if (::melon::CanAnnotateSpan()) {                          \
            ::melon::AnnotateSpan("[" __FILE__ ":" MELON_SYMBOLSTR(__LINE__) "] " fmt, ##args);           \
        }                                                               \
    } while (0)

#endif  // MELON_RPC_TRACEPRINTF_H_
