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


#include <melon/fiber/errno.h>

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
