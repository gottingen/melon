#ifndef ABEL_FORMAT_INTERNAL_PARSER_H_
#define ABEL_FORMAT_INTERNAL_PARSER_H_

#include <climits>
#include <cstddef>
#include <cstdlib>
#include <cassert>
#include <initializer_list>
#include <iosfwd>
#include <iterator>
#include <memory>
#include <vector>
#include <abel/format/internal/checker.h>
#include <abel/format/internal/format_conv.h>

namespace abel {

namespace format_internal {

// The analyzed properties of a single specified conversion.
struct unbound_conversion {
    unbound_conversion ()
        : flags() /* This is required to zero all the fields of flags. */ {
        flags.basic = true;
    }

    class input_value {
    public:
        void set_value (int value) {
            assert(value >= 0);
            _value = value;
        }
        int value () const { return _value; }

        // Marks the value as "from arg". aka the '*' format.
        // Requires `value >= 1`.
        // When set, is_from_arg() return true and get_from_arg() returns the
        // original value.
        // `value()`'s return value is unspecfied in this state.
        void set_from_arg (int value) {
            assert(value > 0);
            _value = -value - 1;
        }
        bool is_from_arg () const { return _value < -1; }
        int get_from_arg () const {
            assert(is_from_arg());
            return -_value - 1;
        }

    private:
        int _value = -1;
    };

    // No need to initialize. It will always be set in the parser.
    int arg_position;

    input_value width;
    input_value precision;

    format_flags flags;
    length_mod length_mod;
    conversion_char conv;
};

// Consume conversion spec prefix (not including '%') of [p, end) if valid.
// Examples of valid specs would be e.g.: "s", "d", "-12.6f".
// If valid, it returns the first character following the conversion spec,
// and the spec part is broken down and returned in 'conv'.
// If invalid, returns nullptr.
const char *consume_unbound_conversion (const char *p, const char *end,
                                        unbound_conversion *conv, int *next_arg);

// Helper tag class for the table below.
// It allows fast `char -> conversion_char/LengthMod` checking and conversions.
class conv_tag {
public:
    constexpr conv_tag (conversion_char::Id id) : _tag(id) { }  // NOLINT
    // We invert the length modifiers to make them negative so that we can easily
    // test for them.
    constexpr conv_tag (length_mod::Id id) : _tag(~id) { }  // NOLINT
    // Everything else is -128, which is negative to make is_conv() simpler.
    constexpr conv_tag () : _tag(-128) { }

    bool is_conv () const { return _tag >= 0; }
    bool is_length () const { return _tag < 0 && _tag != -128; }
    conversion_char as_conv () const {
        assert(is_conv());
        return conversion_char::FromId(static_cast<conversion_char::Id>(_tag));
    }
    length_mod as_length () const {
        assert(is_length());
        return length_mod::FromId(static_cast<length_mod::Id>(~_tag));
    }

private:
    std::int8_t _tag;
};

extern const conv_tag kTags[256];
// Keep a single table for all the conversion chars and length modifiers.
ABEL_FORCE_INLINE conv_tag get_tag_for_char (char c) {
    return kTags[static_cast<unsigned char>(c)];
}

// Parse the format string provided in 'src' and pass the identified items into
// 'consumer'.
// Text runs will be passed by calling
//   Consumer::Append(string_view);
// ConversionItems will be passed by calling
//   Consumer::ConvertOne(unbound_conversion, string_view);
// In the case of ConvertOne, the string_view that is passed is the
// portion of the format string corresponding to the conversion, not including
// the leading %. On success, it returns true. On failure, it stops and returns
// false.
template<typename Consumer>
bool parse_format_string (string_view src, Consumer consumer) {
    int next_arg = 0;
    const char *p = src.data();
    const char *const end = p + src.size();
    while (p != end) {
        const char *percent = static_cast<const char *>(memchr(p, '%', end - p));
        if (!percent) {
            // We found the last substring.
            return consumer.Append(string_view(p, end - p));
        }
        // We found a percent, so push the text run then process the percent.
        if (ABEL_UNLIKELY(!consumer.Append(string_view(p, percent - p)))) {
            return false;
        }
        if (ABEL_UNLIKELY(percent + 1 >= end))
            return false;

        auto tag = get_tag_for_char(percent[1]);
        if (tag.is_conv()) {
            if (ABEL_UNLIKELY(next_arg < 0)) {
                // This indicates an error in the format std::string.
                // The only way to get `next_arg < 0` here is to have a positional
                // argument first which sets next_arg to -1 and then a non-positional
                // argument.
                return false;
            }
            p = percent + 2;

            // Keep this case separate from the one below.
            // ConvertOne is more efficient when the compiler can see that the `basic`
            // flag is set.
            unbound_conversion conv;
            conv.conv = tag.as_conv();
            conv.arg_position = ++next_arg;
            if (ABEL_UNLIKELY(
                !consumer.ConvertOne(conv, string_view(percent + 1, 1)))) {
                return false;
            }
        } else if (percent[1] != '%') {
            unbound_conversion conv;
            p = consume_unbound_conversion(percent + 1, end, &conv, &next_arg);
            if (ABEL_UNLIKELY(p == nullptr))
                return false;
            if (ABEL_UNLIKELY(!consumer.ConvertOne(
                conv, string_view(percent + 1, p - (percent + 1))))) {
                return false;
            }
        } else {
            if (ABEL_UNLIKELY(!consumer.Append("%")))
                return false;
            p = percent + 2;
            continue;
        }
    }
    return true;
}

// Always returns true, or fails to compile in a constexpr context if s does not
// point to a constexpr char array.
constexpr bool ensure_constexpr (string_view s) {
    return s.empty() || s[0] == s[0];
}

class parsed_format_base {
public:
    explicit parsed_format_base (string_view format, bool allow_ignored,
                                 std::initializer_list<format_conv> convs);

    parsed_format_base (const parsed_format_base &other) { *this = other; }

    parsed_format_base (parsed_format_base &&other) { *this = std::move(other); }

    parsed_format_base &operator = (const parsed_format_base &other) {
        if (this == &other)
            return *this;
        has_error_ = other.has_error_;
        items_ = other.items_;
        size_t text_size = items_.empty() ? 0 : items_.back().text_end;
        data_.reset(new char[text_size]);
        memcpy(data_.get(), other.data_.get(), text_size);
        return *this;
    }

    parsed_format_base &operator = (parsed_format_base &&other) {
        if (this == &other)
            return *this;
        has_error_ = other.has_error_;
        data_ = std::move(other.data_);
        items_ = std::move(other.items_);
        // Reset the vector to make sure the invariants hold.
        other.items_.clear();
        return *this;
    }

    template<typename Consumer>
    bool process_format (Consumer consumer) const {
        const char *const base = data_.get();
        string_view text(base, 0);
        for (const auto &item : items_) {
            const char *const end = text.data() + text.size();
            text = string_view(end, (base + item.text_end) - end);
            if (item.is_conversion) {
                if (!consumer.ConvertOne(item.conv, text))
                    return false;
            } else {
                if (!consumer.Append(text))
                    return false;
            }
        }
        return !has_error_;
    }

    bool has_error () const { return has_error_; }

private:
    // Returns whether the conversions match and if !allow_ignored it verifies
    // that all conversions are used by the format.
    bool matches_conversions (bool allow_ignored,
                              std::initializer_list<format_conv> convs) const;

    struct parsed_format_consumer;

    struct conversion_item {
        bool is_conversion;
        // Points to the past-the-end location of this element in the data_ array.
        size_t text_end;
        unbound_conversion conv;
    };

    bool has_error_;
    std::unique_ptr<char[]> data_;
    std::vector<conversion_item> items_;
};

// A value type representing a preparsed format.  These can be created, copied
// around, and reused to speed up formatting loops.
// The user must specify through the template arguments the conversion
// characters used in the format. This will be checked at compile time.
//
// This class uses format_conv enum values to specify each argument.
// This allows for more flexibility as you can specify multiple possible
// conversion characters for each argument.
// parsed_format<char...> is a simplified alias for when the user only
// needs to specify a single conversion character for each argument.
//
// Example:
//   // Extended format supports multiple characters per argument:
//   using MyFormat = extended_parsed_format<format_conv::d | format_conv::x>;
//   MyFormat GetFormat(bool use_hex) {
//     if (use_hex) return MyFormat("foo %x bar");
//     return MyFormat("foo %d bar");
//   }
//   // 'format' can be used with any value that supports 'd' and 'x',
//   // like `int`.
//   auto format = GetFormat(use_hex);
//   value = StringF(format, i);
//
// This class also supports runtime format checking with the ::New() and
// ::NewAllowIgnored() factory functions.
// This is the only API that allows the user to pass a runtime specified format
// string. These factory functions will return NULL if the format does not match
// the conversions requested by the user.
template<format_internal::format_conv... C>
class extended_parsed_format : public format_internal::parsed_format_base {
public:
    explicit extended_parsed_format (string_view format)
#ifdef ABEL_INTERNAL_ENABLE_FORMAT_CHECKER
    __attribute__((
    enable_if(format_internal::ensure_constexpr(format),
    "Format std::string is not constexpr."),
    enable_if(format_internal::valid_format_impl<C...>(format),
    "Format specified does not match the template arguments.")))
#endif  // ABEL_INTERNAL_ENABLE_FORMAT_CHECKER
        : extended_parsed_format(format, false) {
    }

    // extended_parsed_format factory function.
    // The user still has to specify the conversion characters, but they will not
    // be checked at compile time. Instead, it will be checked at runtime.
    // This delays the checking to runtime, but allows the user to pass
    // dynamically sourced formats.
    // It returns NULL if the format does not match the conversion characters.
    // The user is responsible for checking the return value before using it.
    //
    // The 'New' variant will check that all the specified arguments are being
    // consumed by the format and return NULL if any argument is being ignored.
    // The 'NewAllowIgnored' variant will not verify this and will allow formats
    // that ignore arguments.
    static std::unique_ptr<extended_parsed_format> New (string_view format) {
        return New(format, false);
    }
    static std::unique_ptr<extended_parsed_format> NewAllowIgnored (
        string_view format) {
        return New(format, true);
    }

private:
    static std::unique_ptr<extended_parsed_format> New (string_view format,
                                                        bool allow_ignored) {
        std::unique_ptr<extended_parsed_format> conv(
            new extended_parsed_format(format, allow_ignored));
        if (conv->has_error())
            return nullptr;
        return conv;
    }

    extended_parsed_format (string_view s, bool allow_ignored)
        : parsed_format_base(s, allow_ignored, {C...}) { }
};
}  // namespace format_internal

}  // namespace abel

#endif  // ABEL_FORMAT_INTERNAL_PARSER_H_
