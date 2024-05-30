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


#include <stdio.h>
int main() {
#if defined(__clang__)
    const int major_v = __GNUC__;
    int minor_v = __GNUC_MINOR__;
    if (major_v == 4 && minor_v <= 8) {
        // Make version of clang >= 4.8 so that it's not rejected by config_brpc.sh
        minor_v = 8;
    }
    printf("%d\n", (major_v * 10000 + minor_v * 100));
#elif defined(__GNUC__)
    printf("%d\n", (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__));
#else
    printf("0\n");
#endif
    return 0;
}
