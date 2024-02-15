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


#ifndef MELON_FIBER_ERRNO_H_
#define MELON_FIBER_ERRNO_H_

#include <errno.h>                    // errno
#include "melon/utility/errno.h"

__BEGIN_DECLS

extern int *fiber_errno_location();

#ifdef errno
#undef errno
#define errno *fiber_errno_location()
#endif

// List errno used throughout fiber
extern const int ESTOP;

__END_DECLS

#endif  // MELON_FIBER_ERRNO_H_
