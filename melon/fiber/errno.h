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


#ifndef MELON_FIBER_ERRNO_H_
#define MELON_FIBER_ERRNO_H_

#include <errno.h>                    // errno
#include <melon/base/errno.h>

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
