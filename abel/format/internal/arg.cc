//
// POSIX spec:
//   http://pubs.opengroup.org/onlinepubs/009695399/functions/fprintf.html
//
#include <abel/format/internal/arg.h>
#include <cassert>
#include <cerrno>
#include <cstdlib>
#include <string>
#include <type_traits>
#include <abel/base/profile.h>
#include <abel/format/internal/float_conversion.h>

namespace abel {

namespace format_internal {
namespace {

const char kDigit[2][32] = {"0123456789abcdef", "0123456789ABCDEF"};

// Reduce *capacity by s.size(), clipped to a 0 minimum.
void ReducePadding (string_view s, size_t *capacity) {
    *capacity = Excess(s.size(), *capacity);
}

// Reduce *capacity by n, clipped to a 0 minimum.
void ReducePadding (size_t n, size_t *capacity) {
    *capacity = Excess(n, *capacity);
}

template<typename T>
struct MakeUnsigned : std::make_unsigned<T> { };
template<>
struct MakeUnsigned<abel::int128> {
    using type = abel::uint128;
};
template<>
struct MakeUnsigned<abel::uint128> {
    using type = abel::uint128;
};

template<typename T>
struct IsSigned : std::is_signed<T> { };
template<>
struct IsSigned<abel::int128> : std::true_type { };
template<>
struct IsSigned<abel::uint128> : std::false_type { };

class ConvertedIntInfo {
public:
    template<typename T>
    ConvertedIntInfo (T v, conversion_char conv) {
        using Unsigned = typename MakeUnsigned<T>::type;
        auto u = static_cast<Unsigned>(v);
        if (IsNeg(v)) {
            is_neg_ = true;
            u = Unsigned {} - u;
        } else {
            is_neg_ = false;
        }
        UnsignedToStringRight(u, conv);
    }

    string_view digits () const {
        return {end() - size_, static_cast<size_t>(size_)};
    }
    bool is_neg () const { return is_neg_; }

private:
    template<typename T, bool IsSigned>
    struct IsNegImpl {
        static bool Eval (T v) { return v < 0; }
    };
    template<typename T>
    struct IsNegImpl<T, false> {
        static bool Eval (T) {
            return false;
        }
    };

    template<typename T>
    bool IsNeg (T v) {
        return IsNegImpl<T, IsSigned<T>::value>::Eval(v);
    }

    template<typename T>
    void UnsignedToStringRight (T u, conversion_char conv) {
        char *p = end();
        switch (conv.radix()) {
        default:
        case 10:
            for (; u; u /= 10)
                *--p = static_cast<char>('0' + static_cast<size_t>(u % 10));
            break;
        case 8:
            for (; u; u /= 8)
                *--p = static_cast<char>('0' + static_cast<size_t>(u % 8));
            break;
        case 16: {
            const char *digits = kDigit[conv.upper() ? 1 : 0];
            for (; u; u /= 16)
                *--p = digits[static_cast<size_t>(u % 16)];
            break;
        }
        }
        size_ = static_cast<int>(end() - p);
    }

    const char *end () const { return storage_ + sizeof(storage_); }
    char *end () { return storage_ + sizeof(storage_); }

    bool is_neg_;
    int size_;
    // Max size: 128 bit value as octal -> 43 digits
    char storage_[128 / 3 + 1];
};

// Note: 'o' conversions do not have a base indicator, it's just that
// the '#' flag is specified to modify the precision for 'o' conversions.
string_view BaseIndicator (const ConvertedIntInfo &info,
                           const conversion_spec conv) {
    bool alt = conv.flags().alt;
    int radix = conv.conv().radix();
    if (conv.conv().id() == conversion_char::p)
        alt = true;  // always show 0x for %p.
    // From the POSIX description of '#' flag:
    //   "For x or X conversion specifiers, a non-zero result shall have
    //   0x (or 0X) prefixed to it."
    if (alt && radix == 16 && !info.digits().empty()) {
        if (conv.conv().upper())
            return "0X";
        return "0x";
    }
    return {};
}

string_view SignColumn (bool neg, const conversion_spec conv) {
    if (conv.conv().is_signed()) {
        if (neg)
            return "-";
        if (conv.flags().show_pos)
            return "+";
        if (conv.flags().sign_col)
            return " ";
    }
    return {};
}

bool ConvertCharImpl (unsigned char v, const conversion_spec conv,
                      format_sink_impl *sink) {
    size_t fill = 0;
    if (conv.width() >= 0)
        fill = conv.width();
    ReducePadding(1, &fill);
    if (!conv.flags().left)
        sink->Append(fill, ' ');
    sink->Append(1, v);
    if (conv.flags().left)
        sink->Append(fill, ' ');
    return true;
}

bool ConvertIntImplInner (const ConvertedIntInfo &info,
                          const conversion_spec conv, format_sink_impl *sink) {
    // Print as a sequence of Substrings:
    //   [left_spaces][sign][base_indicator][zeroes][formatted][right_spaces]
    size_t fill = 0;
    if (conv.width() >= 0)
        fill = conv.width();

    string_view formatted = info.digits();
    ReducePadding(formatted, &fill);

    string_view sign = SignColumn(info.is_neg(), conv);
    ReducePadding(sign, &fill);

    string_view base_indicator = BaseIndicator(info, conv);
    ReducePadding(base_indicator, &fill);

    int precision = conv.precision();
    bool precision_specified = precision >= 0;
    if (!precision_specified)
        precision = 1;

    if (conv.flags().alt && conv.conv().id() == conversion_char::o) {
        // From POSIX description of the '#' (alt) flag:
        //   "For o conversion, it increases the precision (if necessary) to
        //   force the first digit of the result to be zero."
        if (formatted.empty() || *formatted.begin() != '0') {
            int needed = static_cast<int>(formatted.size()) + 1;
            precision = std::max(precision, needed);
        }
    }

    size_t num_zeroes = Excess(formatted.size(), precision);
    ReducePadding(num_zeroes, &fill);

    size_t num_left_spaces = !conv.flags().left ? fill : 0;
    size_t num_right_spaces = conv.flags().left ? fill : 0;

    // From POSIX description of the '0' (zero) flag:
    //   "For d, i, o, u, x, and X conversion specifiers, if a precision
    //   is specified, the '0' flag is ignored."
    if (!precision_specified && conv.flags().zero) {
        num_zeroes += num_left_spaces;
        num_left_spaces = 0;
    }

    sink->Append(num_left_spaces, ' ');
    sink->Append(sign);
    sink->Append(base_indicator);
    sink->Append(num_zeroes, '0');
    sink->Append(formatted);
    sink->Append(num_right_spaces, ' ');
    return true;
}

template<typename T>
bool ConvertIntImplInner (T v, const conversion_spec conv, format_sink_impl *sink) {
    ConvertedIntInfo info(v, conv.conv());
    if (conv.flags().basic && conv.conv().id() != conversion_char::p) {
        if (info.is_neg())
            sink->Append(1, '-');
        if (info.digits().empty()) {
            sink->Append(1, '0');
        } else {
            sink->Append(info.digits());
        }
        return true;
    }
    return ConvertIntImplInner(info, conv, sink);
}

template<typename T>
bool ConvertIntArg (T v, const conversion_spec conv, format_sink_impl *sink) {
    if (conv.conv().is_float()) {
        return FormatConvertImpl(static_cast<double>(v), conv, sink).value;
    }
    if (conv.conv().id() == conversion_char::c)
        return ConvertCharImpl(static_cast<unsigned char>(v), conv, sink);
    if (!conv.conv().is_integral())
        return false;
    if (!conv.conv().is_signed() && IsSigned<T>::value) {
        using U = typename MakeUnsigned<T>::type;
        return FormatConvertImpl(static_cast<U>(v), conv, sink).value;
    }
    return ConvertIntImplInner(v, conv, sink);
}

template<typename T>
bool ConvertFloatArg (T v, const conversion_spec conv, format_sink_impl *sink) {
    return conv.conv().is_float() && ConvertFloatImpl(v, conv, sink);
}

ABEL_FORCE_INLINE bool ConvertStringArg (string_view v, const conversion_spec conv,
                                         format_sink_impl *sink) {
    if (conv.conv().id() != conversion_char::s)
        return false;
    if (conv.flags().basic) {
        sink->Append(v);
        return true;
    }
    return sink->PutPaddedString(v, conv.width(), conv.precision(),
                                 conv.flags().left);
}

}  // namespace

// ==================== Strings ====================
convert_result<format_conv::s> FormatConvertImpl (const std::string &v,
                                                  const conversion_spec conv,
                                                  format_sink_impl *sink) {
    return {ConvertStringArg(v, conv, sink)};
}

convert_result<format_conv::s> FormatConvertImpl (string_view v,
                                                  const conversion_spec conv,
                                                  format_sink_impl *sink) {
    return {ConvertStringArg(v, conv, sink)};
}

convert_result<format_conv::s | format_conv::p> FormatConvertImpl (const char *v,
                                                                   const conversion_spec conv,
                                                                   format_sink_impl *sink) {
    if (conv.conv().id() == conversion_char::p)
        return {FormatConvertImpl(VoidPtr(v), conv, sink).value};
    size_t len;
    if (v == nullptr) {
        len = 0;
    } else if (conv.precision() < 0) {
        len = std::strlen(v);
    } else {
        // If precision is set, we look for the NUL-terminator on the valid range.
        len = std::find(v, v + conv.precision(), '\0') - v;
    }
    return {ConvertStringArg(string_view(v, len), conv, sink)};
}

// ==================== Raw pointers ====================
convert_result<format_conv::p> FormatConvertImpl (VoidPtr v, const conversion_spec conv,
                                                  format_sink_impl *sink) {
    if (conv.conv().id() != conversion_char::p)
        return {false};
    if (!v.value) {
        sink->Append("(nil)");
        return {true};
    }
    return {ConvertIntImplInner(v.value, conv, sink)};
}

// ==================== Floats ====================
FloatingConvertResult FormatConvertImpl (float v, const conversion_spec conv,
                                         format_sink_impl *sink) {
    return {ConvertFloatArg(v, conv, sink)};
}
FloatingConvertResult FormatConvertImpl (double v, const conversion_spec conv,
                                         format_sink_impl *sink) {
    return {ConvertFloatArg(v, conv, sink)};
}
FloatingConvertResult FormatConvertImpl (long double v,
                                         const conversion_spec conv,
                                         format_sink_impl *sink) {
    return {ConvertFloatArg(v, conv, sink)};
}

// ==================== Chars ====================
IntegralConvertResult FormatConvertImpl (char v, const conversion_spec conv,
                                         format_sink_impl *sink) {
    return {ConvertIntArg(v, conv, sink)};
}
IntegralConvertResult FormatConvertImpl (signed char v,
                                         const conversion_spec conv,
                                         format_sink_impl *sink) {
    return {ConvertIntArg(v, conv, sink)};
}
IntegralConvertResult FormatConvertImpl (unsigned char v,
                                         const conversion_spec conv,
                                         format_sink_impl *sink) {
    return {ConvertIntArg(v, conv, sink)};
}

// ==================== Ints ====================
IntegralConvertResult FormatConvertImpl (short v,  // NOLINT
                                         const conversion_spec conv,
                                         format_sink_impl *sink) {
    return {ConvertIntArg(v, conv, sink)};
}
IntegralConvertResult FormatConvertImpl (unsigned short v,  // NOLINT
                                         const conversion_spec conv,
                                         format_sink_impl *sink) {
    return {ConvertIntArg(v, conv, sink)};
}
IntegralConvertResult FormatConvertImpl (int v, const conversion_spec conv,
                                         format_sink_impl *sink) {
    return {ConvertIntArg(v, conv, sink)};
}
IntegralConvertResult FormatConvertImpl (unsigned v, const conversion_spec conv,
                                         format_sink_impl *sink) {
    return {ConvertIntArg(v, conv, sink)};
}
IntegralConvertResult FormatConvertImpl (long v,  // NOLINT
                                         const conversion_spec conv,
                                         format_sink_impl *sink) {
    return {ConvertIntArg(v, conv, sink)};
}
IntegralConvertResult FormatConvertImpl (unsigned long v,  // NOLINT
                                         const conversion_spec conv,
                                         format_sink_impl *sink) {
    return {ConvertIntArg(v, conv, sink)};
}
IntegralConvertResult FormatConvertImpl (long long v,  // NOLINT
                                         const conversion_spec conv,
                                         format_sink_impl *sink) {
    return {ConvertIntArg(v, conv, sink)};
}
IntegralConvertResult FormatConvertImpl (unsigned long long v,  // NOLINT
                                         const conversion_spec conv,
                                         format_sink_impl *sink) {
    return {ConvertIntArg(v, conv, sink)};
}
IntegralConvertResult FormatConvertImpl (abel::int128 v,
                                         const conversion_spec conv,
                                         format_sink_impl *sink) {
    return {ConvertIntArg(v, conv, sink)};
}
IntegralConvertResult FormatConvertImpl (abel::uint128 v,
                                         const conversion_spec conv,
                                         format_sink_impl *sink) {
    return {ConvertIntArg(v, conv, sink)};
}

ABEL_INTERNAL_FORMAT_DISPATCH_OVERLOADS_EXPAND_();

}  // namespace format_internal


}  // namespace abel
