//
// Created by liyinbin on 2020/1/30.
//

#ifndef ABEL_SYSTEM_TERMINAL_H_
#define ABEL_SYSTEM_TERMINAL_H_
#include <abel/system/platform_headers.h>
namespace abel {

// Determine if the terminal supports colors
// Source: https://github.com/agauniyal/rang/
inline bool is_color_terminal () {
#ifdef _WIN32
    return true;
#else
    static constexpr const char *Terms[] = {
        "ansi", "color", "console", "cygwin", "gnome", "konsole", "kterm", "linux", "msys", "putty", "rxvt", "screen",
        "vt100", "xterm"};

    const char *env_p = std::getenv("TERM");
    if (env_p == nullptr) {
        return false;
    }

    static const bool result =
        std::any_of(std::begin(Terms),
                    std::end(Terms),
                    [&] (const char *term) { return std::strstr(env_p, term) != nullptr; });
    return result;
#endif
}

// Detrmine if the terminal attached
// Source: https://github.com/agauniyal/rang/
inline bool in_terminal (FILE *file) {

#ifdef _WIN32
    return _isatty(_fileno(file)) != 0;
#else
    return isatty(fileno(file)) != 0;
#endif
}

} //namespace abel
#endif //ABEL_SYSTEM_TERMINAL_H_
