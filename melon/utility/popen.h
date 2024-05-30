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


// Date: 2017/11/04 17:13:18

#ifndef  PUBLIC_COMMON_POPEN_H
#define  PUBLIC_COMMON_POPEN_H

#include <ostream>

namespace mutil {

// Read the stdout of child process executing `cmd'.
// Returns the exit status(0-255) of cmd and all the output is stored in
// |os|. -1 otherwise and errno is set appropriately.
int read_command_output(std::ostream& os, const char* cmd);

}

#endif  //PUBLIC_COMMON_POPEN_H
