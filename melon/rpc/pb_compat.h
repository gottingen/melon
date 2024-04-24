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



#ifndef MELON_RPC_PB_COMPAT_H_
#define MELON_RPC_PB_COMPAT_H_

#if GOOGLE_PROTOBUF_VERSION < 3021000
# define PB_321_OVERRIDE override
#else
# define PB_321_OVERRIDE
#endif

#if GOOGLE_PROTOBUF_VERSION < 3019000
# define PB_319_OVERRIDE override
#else
# define PB_319_OVERRIDE
#endif

#if GOOGLE_PROTOBUF_VERSION < 3010000
# define PB_310_OVERRIDE override
#else
# define PB_310_OVERRIDE
#endif

#endif  // MELON_RPC_PB_COMPAT_H_
