#ifndef ABEL_FORMAT_INTERNAL_CHECKER_H_
#define ABEL_FORMAT_INTERNAL_CHECKER_H_

#include <abel/base/profile.h>
#include <abel/format/internal/arg.h>
#include <abel/format/internal/format_conv.h>

// Compile time check support for entry points.

#ifndef ABEL_INTERNAL_ENABLE_FORMAT_CHECKER
#if ABEL_COMPILER_HAS_ATTRIBUTE(enable_if) && !defined(__native_client__)
#define ABEL_INTERNAL_ENABLE_FORMAT_CHECKER 1
#endif  // ABEL_COMPILER_HAS_ATTRIBUTE(enable_if) && !defined(__native_client__)
#endif  // ABEL_INTERNAL_ENABLE_FORMAT_CHECKER

namespace abel {

namespace format_internal {

constexpr bool AllOf () { return true; }

template<typename... T>
constexpr bool AllOf (bool b, T... t) {
    return b && AllOf(t...);
}

template<typename Arg>
constexpr format_conv argument_to_conv () {
    return decltype(format_internal::FormatConvertImpl(
        std::declval<const Arg &>(), std::declval<const conversion_spec &>(),
        std::declval<format_sink_impl *>()))::kConv;
}

#ifdef ABEL_INTERNAL_ENABLE_FORMAT_CHECKER

constexpr bool contains_char (const char *chars, char c) {
    return *chars == c || (*chars && contains_char(chars + 1, c));
}

// A constexpr compatible list of Convs.
struct conv_list {
    const format_conv *array;
    int count;

    // We do the bound check here to avoid having to do it on the callers.
    // Returning an empty format_conv has the same effect as short circuiting because it
    // will never match any conversion.
    constexpr format_conv operator [] (int i) const {
        return i < count ? array[i] : format_conv {};
    }

    constexpr conv_list without_front () const {
        return count != 0 ? conv_list {array + 1, count - 1} : *this;
    }
};

template<size_t count>
struct conv_list_t {
    // Make sure the array has size > 0.
    format_conv list[count ? count : 1];
};

constexpr char GetChar (string_view str, size_t index) {
    return index < str.size() ? str[index] : char {};
}

constexpr string_view consume_front (string_view str, size_t len = 1) {
    return len <= str.size() ? string_view(str.data() + len, str.size() - len)
                             : string_view();
}

constexpr string_view consume_any_of (string_view format, const char *chars) {
    return contains_char(chars, GetChar(format, 0))
           ? consume_any_of(consume_front(format), chars)
           : format;
}

constexpr bool IsDigit (char c) { return c >= '0' && c <= '9'; }

// Helper class for the ParseDigits function.
// It encapsulates the two return values we need there.
struct checker_integer {
    string_view format;
    int value;

    // If the next character is a '$', consume it.
    // Otherwise, make `this` an invalid positional argument.
    constexpr checker_integer ConsumePositionalDollar () const {
        return GetChar(format, 0) == '$' ? checker_integer {consume_front(format), value}
                                         : checker_integer {format, 0};
    }
};

constexpr checker_integer parse_digits (string_view format, int value = 0) {
    return IsDigit(GetChar(format, 0))
           ? parse_digits(consume_front(format),
                          10 * value + GetChar(format, 0) - '0')
           : checker_integer {format, value};
}

// parse digits for a positional argument.
// The parsing also consumes the '$'.
constexpr checker_integer parse_positional (string_view format) {
    return parse_digits(format).ConsumePositionalDollar();
}

// Parses a single conversion specifier.
// See conv_parser::Run() for post conditions.
class conv_parser {
    constexpr conv_parser set_format (string_view format) const {
        return conv_parser(format, args_, error_, arg_position_, is_positional_);
    }

    constexpr conv_parser set_args (conv_list args) const {
        return conv_parser(format_, args, error_, arg_position_, is_positional_);
    }

    constexpr conv_parser set_error (bool error) const {
        return conv_parser(format_, args_, error_ || error, arg_position_,
                           is_positional_);
    }

    constexpr conv_parser set_arg_position (int arg_position) const {
        return conv_parser(format_, args_, error_, arg_position, is_positional_);
    }

    // Consumes the next arg and verifies that it matches `conv`.
    // `error_` is set if there is no next arg or if it doesn't match `conv`.
    constexpr conv_parser consume_next_arg (char conv) const {
        return set_args(args_.without_front()).set_error(!conv_contains(args_[0], conv));
    }

    // Verify that positional argument `i.value` matches `conv`.
    // `error_` is set if `i.value` is not a valid argument or if it doesn't
    // match.
    constexpr conv_parser verify_positional (checker_integer i, char conv) const {
        return set_format(i.format).set_error(!conv_contains(args_[i.value - 1], conv));
    }

    // parse the position of the arg and store it in `arg_position_`.
    constexpr conv_parser parse_arg_position (checker_integer arg) const {
        return set_format(arg.format).set_arg_position(arg.value);
    }

    // Consume the flags.
    constexpr conv_parser parse_flags () const {
        return set_format(consume_any_of(format_, "-+ #0"));
    }

    // Consume the width.
    // If it is '*', we verify that it matches `args_`. `error_` is set if it
    // doesn't match.
    constexpr conv_parser parse_width () const {
        return IsDigit(GetChar(format_, 0))
               ? set_format(parse_digits(format_).format)
               : GetChar(format_, 0) == '*'
                 ? is_positional_
                   ? verify_positional(
                        parse_positional(consume_front(format_)), '*')
                   : set_format(consume_front(format_))
                       .consume_next_arg('*')
                 : *this;
    }

    // Consume the precision.
    // If it is '*', we verify that it matches `args_`. `error_` is set if it
    // doesn't match.
    constexpr conv_parser parse_precision () const {
        return GetChar(format_, 0) != '.'
               ? *this
               : GetChar(format_, 1) == '*'
                 ? is_positional_
                   ? verify_positional(
                        parse_positional(consume_front(format_, 2)), '*')
                   : set_format(consume_front(format_, 2))
                       .consume_next_arg('*')
                 : set_format(parse_digits(consume_front(format_)).format);
    }

    // Consume the length characters.
    constexpr conv_parser parse_length () const {
        return set_format(consume_any_of(format_, "lLhjztq"));
    }

    // Consume the conversion character and verify that it matches `args_`.
    // `error_` is set if it doesn't match.
    constexpr conv_parser parse_conversion () const {
        return is_positional_
               ? verify_positional({consume_front(format_), arg_position_},
                                   GetChar(format_, 0))
               : consume_next_arg(GetChar(format_, 0))
                   .set_format(consume_front(format_));
    }

    constexpr conv_parser (string_view format, conv_list args, bool error,
                           int arg_position, bool is_positional)
        : format_(format),
          args_(args),
          error_(error),
          arg_position_(arg_position),
          is_positional_(is_positional) { }

public:
    constexpr conv_parser (string_view format, conv_list args, bool is_positional)
        : format_(format),
          args_(args),
          error_(false),
          arg_position_(0),
          is_positional_(is_positional) { }

    // Consume the whole conversion specifier.
    // `format()` will be set to the character after the conversion character.
    // `error()` will be set if any of the arguments do not match.
    constexpr conv_parser run () const {
        return (is_positional_ ? parse_arg_position(parse_positional(format_)) : *this)
            .parse_flags()
            .parse_width()
            .parse_precision()
            .parse_length()
            .parse_conversion();
    }

    constexpr string_view format () const { return format_; }
    constexpr conv_list args () const { return args_; }
    constexpr bool error () const { return error_; }
    constexpr bool is_positional () const { return is_positional_; }

private:
    string_view format_;
    // Current list of arguments. If we are not in positional mode we will consume
    // from the front.
    conv_list args_;
    bool error_;
    // Holds the argument position of the conversion character, if we are in
    // positional mode. Otherwise, it is unspecified.
    int arg_position_;
    // Whether we are in positional mode.
    // It changes the behavior of '*' and where to find the converted argument.
    bool is_positional_;
};

// Parses a whole format expression.
// See FormatParser::Run().
class format_parser {
    static constexpr bool found_percent (string_view format) {
        return format.empty() ||
            (GetChar(format, 0) == '%' && GetChar(format, 1) != '%');
    }

    // We use an inner function to increase the recursion limit.
    // The inner function consumes up to `limit` characters on every run.
    // This increases the limit from 512 to ~512*limit.
    static constexpr string_view ConsumeNonPercentInner (string_view format,
                                                         int limit = 20) {
        return found_percent(format) || !limit
               ? format
               : ConsumeNonPercentInner(
                consume_front(format, GetChar(format, 0) == '%' &&
                    GetChar(format, 1) == '%'
                                      ? 2
                                      : 1),
                limit - 1);
    }

    // Consume characters until the next conversion spec %.
    // It skips %%.
    static constexpr string_view consume_non_percent (string_view format) {
        return found_percent(format)
               ? format
               : consume_non_percent(ConsumeNonPercentInner(format));
    }

    static constexpr bool is_positional (string_view format) {
        return IsDigit(GetChar(format, 0)) ? is_positional(consume_front(format))
                                           : GetChar(format, 0) == '$';
    }

    constexpr bool run_impl (bool is_positional) const {
        // In non-positional mode we require all arguments to be consumed.
        // In positional mode just reaching the end of the format without errors is
        // enough.
        return (format_.empty() && (is_positional || args_.count == 0)) ||
            (!format_.empty() &&
                validate_arg(
                    conv_parser(consume_front(format_), args_, is_positional).run()));
    }

    constexpr bool validate_arg (conv_parser conv) const {
        return !conv.error() && format_parser(conv.format(), conv.args())
            .run_impl(conv.is_positional());
    }

public:
    constexpr format_parser (string_view format, conv_list args)
        : format_(consume_non_percent(format)), args_(args) { }

    // Runs the parser for `format` and `args`.
    // It verifies that the format is valid and that all conversion specifiers
    // match the arguments passed.
    // In non-positional mode it also verfies that all arguments are consumed.
    constexpr bool run () const {
        return run_impl(!format_.empty() && is_positional(consume_front(format_)));
    }

private:
    string_view format_;
    // Current list of arguments.
    // If we are not in positional mode we will consume from the front and will
    // have to be empty in the end.
    conv_list args_;
};

template<format_conv... C>
constexpr bool valid_format_impl (string_view format) {
    return format_parser(format,
                         {conv_list_t<sizeof...(C)> {{C...}}.list, sizeof...(C)})
        .run();
}

#endif  // ABEL_INTERNAL_ENABLE_FORMAT_CHECKER

}  // namespace format_internal

}  // namespace abel

#endif  // ABEL_FORMAT_INTERNAL_CHECKER_H_
