//
// Created by liyinbin on 2020/4/29.
//

#ifndef ABEL_STRINGS_STRING_SPRINTF_H_
#define ABEL_STRINGS_STRING_SPRINTF_H_

#include <string>
#include <stdarg.h>

namespace abel {
    // Convert |format| and associated arguments to std::string
    std::string string_printf(const char *format, ...)
    __attribute__ ((format (printf, 1, 2)));

    // Write |format| and associated arguments into |output|
    // Returns 0 on success, -1 otherwise.
    int string_printf(std::string *output, const char *fmt, ...)
    __attribute__ ((format (printf, 2, 3)));

    // Write |format| and associated arguments in form of va_list into |output|.
    // Returns 0 on success, -1 otherwise.
    int string_vprintf(std::string *output, const char *format, va_list args);

    // Append |format| and associated arguments to |output|
    // Returns 0 on success, -1 otherwise.
    int string_appendf(std::string *output, const char *format, ...)
    __attribute__ ((format (printf, 2, 3)));

    // Append |format| and associated arguments in form of va_list to |output|.
    // Returns 0 on success, -1 otherwise.
    int string_vappendf(std::string *output, const char *format, va_list args);
}
#endif //ABEL_STRINGS_STRING_SPRINTF_H_
