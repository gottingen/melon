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
