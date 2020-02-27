//

#ifndef ABEL_LOG_LOG_SEVERITY_H_
#define ABEL_LOG_LOG_SEVERITY_H_

#include <array>
#include <ostream>
#include <abel/base/profile.h>

namespace abel {


// abel::LogSeverity
//
// Four severity levels are defined. Logging APIs should terminate the program
// when a message is logged at severity `kFatal`; the other levels have no
// special semantics.
//
// Values other than the four defined levels (e.g. produced by `static_cast`)
// are valid, but their semantics when passed to a function, macro, or flag
// depend on the function, macro, or flag. The usual behavior is to normalize
// such values to a defined severity level, however in some cases values other
// than the defined levels are useful for comparison.
//
// Exmaple:
//
//   // Effectively disables all logging:
//   SetMinLogLevel(static_cast<abel::LogSeverity>(100));
//
// abel flags may be defined with type `LogSeverity`. Dependency layering
// constraints require that the `abel_parse_flag()` overload be declared and
// defined in the flags library itself rather than here. The `abel_unparse_flag()`
// overload is defined there as well for consistency.
//
// abel::LogSeverity Flag String Representation
//
// An `abel::LogSeverity` has a string representation used for parsing
// command-line flags based on the enumerator name (e.g. `kFatal`) or
// its unprefixed name (without the `k`) in any case-insensitive form. (E.g.
// "FATAL", "fatal" or "Fatal" are all valid.) Unparsing such flags produces an
// unprefixed string representation in all caps (e.g. "FATAL") or an integer.
//
// Additionally, the parser accepts arbitrary integers (as if the type were
// `int`).
//
// Examples:
//
//   --my_log_level=kInfo
//   --my_log_level=INFO
//   --my_log_level=info
//   --my_log_level=0
//
// Unparsing a flag produces the same result as `abel::LogSeverityName()` for
// the standard levels and a base-ten integer otherwise.
    enum class LogSeverity : int {
        kInfo = 0,
        kWarning = 1,
        kError = 2,
        kFatal = 3,
    };

// LogSeverities()
//
// Returns an iterable of all standard `abel::LogSeverity` values, ordered from
// least to most severe.
    constexpr std::array<abel::LogSeverity, 4> LogSeverities() {
        return {{abel::LogSeverity::kInfo, abel::LogSeverity::kWarning,
                        abel::LogSeverity::kError, abel::LogSeverity::kFatal}};
    }

// LogSeverityName()
//
// Returns the all-caps string representation (e.g. "INFO") of the specified
// severity level if it is one of the standard levels and "UNKNOWN" otherwise.
    constexpr const char *LogSeverityName(abel::LogSeverity s) {
        return s == abel::LogSeverity::kInfo
               ? "INFO"
               : s == abel::LogSeverity::kWarning
                 ? "WARNING"
                 : s == abel::LogSeverity::kError
                   ? "ERROR"
                   : s == abel::LogSeverity::kFatal ? "FATAL" : "UNKNOWN";
    }

// NormalizeLogSeverity()
//
// Values less than `kInfo` normalize to `kInfo`; values greater than `kFatal`
// normalize to `kError` (**NOT** `kFatal`).
    constexpr abel::LogSeverity NormalizeLogSeverity(abel::LogSeverity s) {
        return s < abel::LogSeverity::kInfo
               ? abel::LogSeverity::kInfo
               : s > abel::LogSeverity::kFatal ? abel::LogSeverity::kError : s;
    }

    constexpr abel::LogSeverity NormalizeLogSeverity(int s) {
        return abel::NormalizeLogSeverity(static_cast<abel::LogSeverity>(s));
    }

// operator<<
//
// The exact representation of a streamed `abel::LogSeverity` is deliberately
// unspecified; do not rely on it.
    std::ostream &operator<<(std::ostream &os, abel::LogSeverity s);


}  // namespace abel

#endif  // ABEL_LOG_LOG_SEVERITY_H_
