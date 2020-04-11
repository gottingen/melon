//
// Created by liyinbin on 2020/4/7.
//

#ifndef ABEL_SYSTEM_ERROR_CODE_H_
#define ABEL_SYSTEM_ERROR_CODE_H_

namespace abel {

    extern int describe_customized_errno(int, const char*, const char*);

    template <int error_code>
    class abel_errno_helper {};

    const char* abel_error(int error_code);
    const char* abel_error();

} //namespace abel

#define ABEL_REGISTER_ERRNO(error_code, description)                   \
    const int ABEL_ALLOW_UNUSED ABEL_CONCAT(abel_errno_dummy_, __LINE__) =              \
        ::abel::describe_customized_errno((error_code), #error_code, (description)); \
    template <> class ::abel::abel_errno_helper<(int)(error_code)> {};

#endif //ABEL_SYSTEM_ERROR_CODE_H_
