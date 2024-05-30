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


#pragma once


namespace melon {

// Check if the kernel supports EPOLLRDHUP which is added in Linux 2.6.17
// This flag is useful in Edge Triggered mode. Without the flag user has
// to call an additional read() even if return value(positive) is less
// than given `count', otherwise return value=0(indicating EOF) may be lost.
extern const unsigned int has_epollrdhup;

} // namespace melon

