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

 
// Date: Fri Sep 10 13:34:25 CST 2010

// Add customized errno.

#ifndef MELON_UTILITY_BERRNO_H_
#define MELON_UTILITY_BERRNO_H_

#ifndef __const__
// Avoid over-optimizations of TLS variables by GCC>=4.8
#define __const__ __unused__
#endif

#include <errno.h>                           // errno
#include <melon/base/macros.h>                     // MELON_CONCAT

//-----------------------------------------
// Use system errno before defining yours !
//-----------------------------------------
//
// To add new errno, you shall define the errno in header first, either by
// macro or constant, or even in protobuf.
//
//     #define ESTOP -114                // C/C++
//     static const int EMYERROR = 30;   // C/C++
//     const int EMYERROR2 = -31;        // C++ only
//
// Then you can register description of the error by calling
// MELON_REGISTER_ERRNO(the_error_number, its_description) in global scope of
// a .cpp or .cc files which will be linked.
// 
//     MELON_REGISTER_ERRNO(ESTOP, "the thread is stopping")
//     MELON_REGISTER_ERRNO(EMYERROR, "my error")
//
// Once the error is successfully defined:
//     berror(error_code) returns the description.
//     berror() returns description of last system error code.
//
// %m in printf-alike functions does NOT recognize errors defined by
// MELON_REGISTER_ERRNO, you have to explicitly print them by %s.
//
//     errno = ESTOP;
//     printf("Something got wrong, %m\n");            // NO
//     printf("Something got wrong, %s\n", berror());  // YES
//
// When the error number is re-defined, a linking error will be reported:
// 
//     "redefinition of `class BaiduErrnoHelper<30>'"
//
// Or the program aborts at runtime before entering main():
// 
//     "Fail to define EMYERROR(30) which is already defined as `Read-only file system', abort"
//

namespace mutil {
// You should not call this function, use MELON_REGISTER_ERRNO instead.
extern int DescribeCustomizedErrno(int, const char*, const char*);
}

template <int error_code> class BaiduErrnoHelper {};

#define MELON_REGISTER_ERRNO(error_code, description)                   \
    const int ALLOW_UNUSED MELON_CONCAT(melon_errno_dummy_, __LINE__) =              \
        ::mutil::DescribeCustomizedErrno((error_code), #error_code, (description)); \
    template <> class BaiduErrnoHelper<(int)(error_code)> {};

const char* berror(int error_code);
const char* berror();

#endif  // MELON_UTILITY_BERRNO_H_
