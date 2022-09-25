
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/

#ifndef MELON_BASE_ERRNO_H_
#define MELON_BASE_ERRNO_H_

#define __const__

#include <errno.h>                           // errno
#include "melon/base/profile.h"

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
//     melon_error(error_code) returns the description.
//     melon_error() returns description of last system error code.
//
// %m in printf-alike functions does NOT recognize errors defined by
// MELON_REGISTER_ERRNO, you have to explicitly print them by %s.
//
//     errno = ESTOP;
//     printf("Something got wrong, %m\n");            // NO
//     printf("Something got wrong, %s\n", melon_error());  // YES
//
// When the error number is re-defined, a linking error will be reported:
// 
//     "redefinition of `class melon_errno_helper<30>'"
//
// Or the program aborts at runtime before entering main():
// 
//     "Fail to define EMYERROR(30) which is already defined as `Read-only file system', abort"
//

namespace melon::base {
    // You should not call this function, use MELON_REGISTER_ERRNO instead.
    extern int describe_customized_errno(int, const char *, const char *);
}  // namespace melon::base

template<int error_code>
class melon_errno_helper {
};

#define MELON_REGISTER_ERRNO(error_code, description)                   \
    const int MELON_ALLOW_UNUSED MELON_CONCAT(melon_errno_dummy_, __LINE__) =              \
        ::melon::base::describe_customized_errno((error_code), #error_code, (description)); \
    template <> class melon_errno_helper<(int)(error_code)> {};

const char *melon_error(int error_code);

const char *melon_error();

#endif  // MELON_BASE_ERRNO_H_
