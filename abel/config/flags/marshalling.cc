//

#include <abel/config/flags/marshalling.h>

#include <limits>
#include <type_traits>

#include <abel/base/profile.h>
#include <abel/asl/ascii.h>
#include <abel/strings/ends_with.h>
#include <abel/strings/compare.h>
#include <abel/strings/numbers.h>
#include <abel/strings/str_cat.h>
#include <abel/asl/format/printf.h>
#include <abel/strings/str_join.h>
#include <abel/strings/str_split.h>

namespace abel {

    namespace flags_internal {

// --------------------------------------------------------------------
// abel_parse_flag specializations for boolean type.

        bool abel_parse_flag(abel::string_view text, bool *dst, std::string *) {
            const char *kTrue[] = {"1", "t", "true", "y", "yes"};
            const char *kFalse[] = {"0", "f", "false", "n", "no"};
            static_assert(sizeof(kTrue) == sizeof(kFalse), "true_false_equal");

            text = abel::trim_all(text);

            for (size_t i = 0; i < ABEL_ARRAYSIZE(kTrue); ++i) {
                if (abel::equal_case(text, kTrue[i])) {
                    *dst = true;
                    return true;
                } else if (abel::equal_case(text, kFalse[i])) {
                    *dst = false;
                    return true;
                }
            }
            return false;  // didn't match a legal input
        }

// --------------------------------------------------------------------
// abel_parse_flag for integral types.

// Return the base to use for parsing text as an integer.  Leading 0x
// puts us in base 16.  But leading 0 does not put us in base 8. It
// caused too many bugs when we had that behavior.
        static int NumericBase(abel::string_view text) {
            const bool hex = (text.size() >= 2 && text[0] == '0' &&
                              (text[1] == 'x' || text[1] == 'X'));
            return hex ? 16 : 10;
        }

        template<typename IntType>
        ABEL_FORCE_INLINE bool ParseFlagImpl(abel::string_view text, IntType *dst) {
            text = abel::trim_all(text);

            return abel::numbers_internal::safe_strtoi_base(text, dst, NumericBase(text));
        }

        bool abel_parse_flag(abel::string_view text, short *dst, std::string *) {
            int val;
            if (!ParseFlagImpl(text, &val)) return false;
            if (static_cast<short>(val) != val)  // worked, but number out of range
                return false;
            *dst = static_cast<short>(val);
            return true;
        }

        bool abel_parse_flag(abel::string_view text, unsigned short *dst, std::string *) {
            unsigned int val;
            if (!ParseFlagImpl(text, &val)) return false;
            if (static_cast<unsigned short>(val) !=
                val)  // worked, but number out of range
                return false;
            *dst = static_cast<unsigned short>(val);
            return true;
        }

        bool abel_parse_flag(abel::string_view text, int *dst, std::string *) {
            return ParseFlagImpl(text, dst);
        }

        bool abel_parse_flag(abel::string_view text, unsigned int *dst, std::string *) {
            return ParseFlagImpl(text, dst);
        }

        bool abel_parse_flag(abel::string_view text, long *dst, std::string *) {
            return ParseFlagImpl(text, dst);
        }

        bool abel_parse_flag(abel::string_view text, unsigned long *dst, std::string *) {
            return ParseFlagImpl(text, dst);
        }

        bool abel_parse_flag(abel::string_view text, long long *dst, std::string *) {
            return ParseFlagImpl(text, dst);
        }

        bool abel_parse_flag(abel::string_view text, unsigned long long *dst,
                             std::string *) {
            return ParseFlagImpl(text, dst);
        }

// --------------------------------------------------------------------
// abel_parse_flag for floating point types.

        bool abel_parse_flag(abel::string_view text, float *dst, std::string *) {
            return abel::simple_atof(text, dst);
        }

        bool abel_parse_flag(abel::string_view text, double *dst, std::string *) {
            return abel::simple_atod(text, dst);
        }

// --------------------------------------------------------------------
// abel_parse_flag for strings.

        bool abel_parse_flag(abel::string_view text, std::string *dst, std::string *) {
            dst->assign(text.data(), text.size());
            return true;
        }

// --------------------------------------------------------------------
// abel_parse_flag for vector of strings.

        bool abel_parse_flag(abel::string_view text, std::vector<std::string> *dst,
                             std::string *) {
            // An empty flag value corresponds to an empty vector, not a vector
            // with a single, empty std::string.
            if (text.empty()) {
                dst->clear();
                return true;
            }
            *dst = abel::string_split(text, ',', abel::allow_empty());
            return true;
        }

// --------------------------------------------------------------------
// abel_unparse_flag specializations for various builtin flag types.

        std::string unparse(bool v) { return v ? "true" : "false"; }

        std::string unparse(short v) { return abel::string_cat(v); }

        std::string unparse(unsigned short v) { return abel::string_cat(v); }

        std::string unparse(int v) { return abel::string_cat(v); }

        std::string unparse(unsigned int v) { return abel::string_cat(v); }

        std::string unparse(long v) { return abel::string_cat(v); }

        std::string unparse(unsigned long v) { return abel::string_cat(v); }

        std::string unparse(long long v) { return abel::string_cat(v); }

        std::string unparse(unsigned long long v) { return abel::string_cat(v); }

        template<typename T>
        std::string UnparseFloatingPointVal(T v) {
            // digits10 is guaranteed to roundtrip correctly in std::string -> value -> std::string
            // conversions, but may not be enough to represent all the values correctly.
            std::string digit10_str =
                    fmt::sprintf("%.*g", std::numeric_limits<T>::digits10, v);
            if (std::isnan(v) || std::isinf(v)) return digit10_str;

            T roundtrip_val = 0;
            std::string err;
            if (abel::parse_flag(digit10_str, &roundtrip_val, &err) &&
                roundtrip_val == v) {
                return digit10_str;
            }

            // max_digits10 is the number of base-10 digits that are necessary to uniquely
            // represent all distinct values.
            return fmt::sprintf("%.*g", std::numeric_limits<T>::max_digits10, v);
        }

        std::string unparse(float v) { return UnparseFloatingPointVal(v); }

        std::string unparse(double v) { return UnparseFloatingPointVal(v); }

        std::string abel_unparse_flag(abel::string_view v) { return std::string(v); }

        std::string abel_unparse_flag(const std::vector<std::string> &v) {
            return abel::string_join(v, ",");
        }

    }  // namespace flags_internal

    bool abel_parse_flag(abel::string_view text, abel::LogSeverity *dst,
                         std::string *err) {
        text = abel::trim_all(text);
        if (text.empty()) {
            *err = "no value provided";
            return false;
        }
        if (text.front() == 'k' || text.front() == 'K') text.remove_prefix(1);
        if (abel::equal_case(text, "info")) {
            *dst = abel::LogSeverity::kInfo;
            return true;
        }
        if (abel::equal_case(text, "warning")) {
            *dst = abel::LogSeverity::kWarning;
            return true;
        }
        if (abel::equal_case(text, "error")) {
            *dst = abel::LogSeverity::kError;
            return true;
        }
        if (abel::equal_case(text, "fatal")) {
            *dst = abel::LogSeverity::kFatal;
            return true;
        }
        std::underlying_type<abel::LogSeverity>::type numeric_value;
        if (abel::parse_flag(text, &numeric_value, err)) {
            *dst = static_cast<abel::LogSeverity>(numeric_value);
            return true;
        }
        *err = "only integers and abel::LogSeverity enumerators are accepted";
        return false;
    }

    std::string abel_unparse_flag(abel::LogSeverity v) {
        if (v == abel::NormalizeLogSeverity(v)) return abel::LogSeverityName(v);
        return abel::unparse_flag(static_cast<int>(v));
    }


}  // namespace abel
