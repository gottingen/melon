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


#include "melon/fiber/errno.h"

// Define errno in fiber/errno.h
extern const int ESTOP = -20;

MELON_REGISTER_ERRNO(ESTOP, "The structure is stopping")

extern "C" {

#if defined(OS_LINUX)

extern int *__errno_location() __attribute__((__const__));

int *fiber_errno_location() {
    return __errno_location();
}
#elif defined(OS_MACOSX)

extern int * __error(void);

int *fiber_errno_location() {
    return __error();
}
#endif

}  // extern "C"
