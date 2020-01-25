#ifndef ABEL_FORMAT_INTERNAL_BIND_H_
#define ABEL_FORMAT_INTERNAL_BIND_H_

#include <array>
#include <cstdio>
#include <sstream>
#include <string>
#include <abel/base/profile.h>
#include <abel/format/internal/arg.h>
#include <abel/format/internal/checker.h>
#include <abel/format/internal/parser.h>
#include <abel/types/span.h>

namespace abel {

class untyped_format_spec;

namespace format_internal {

class bound_conversion : public conversion_spec {
public:
    const format_arg_impl *arg () const { return _arg; }
    void set_arg (const format_arg_impl *a) { _arg = a; }

private:
    const format_arg_impl *_arg;
};

// This is the type-erased class that the implementation uses.
class untyped_format_spec_impl {
public:
    untyped_format_spec_impl () = delete;

    explicit untyped_format_spec_impl (string_view s)
        : _data(s.data()), _size(s.size()) { }
    explicit untyped_format_spec_impl (
        const format_internal::parsed_format_base *pc)
        : _data(pc), _size(~size_t {}) { }

    bool has_parsed_conversion () const { return _size == ~size_t {}; }

    string_view str () const {
        assert(!has_parsed_conversion());
        return string_view(static_cast<const char *>(_data), _size);
    }
    const format_internal::parsed_format_base *parsed_conversion () const {
        assert(has_parsed_conversion());
        return static_cast<const format_internal::parsed_format_base *>(_data);
    }

    template<typename T>
    static const untyped_format_spec_impl &Extract (const T &s) {
        return s.get_spec();
    }

private:
    const void *_data;
    size_t _size;
};

template<typename T, typename...>
struct make_dependent {
    using type = T;
};

// Implicitly convertible from `const char*`, `string_view`, and the
// `extended_parsed_format` type. This abstraction allows all format functions to
// operate on any without providing too many overloads.
template<typename... Args>
class format_spec_template
    : public make_dependent<untyped_format_spec, Args...>::type {
    using Base = typename make_dependent<untyped_format_spec, Args...>::type;

public:
#ifdef ABEL_INTERNAL_ENABLE_FORMAT_CHECKER

    // Honeypot overload for when the std::string is not constexpr.
    // We use the 'unavailable' attribute to give a better compiler error than
    // just 'method is deleted'.
    format_spec_template (...)  // NOLINT
    __attribute__((unavailable("Format std::string is not constexpr.")));

    // Honeypot overload for when the format is constexpr and invalid.
    // We use the 'unavailable' attribute to give a better compiler error than
    // just 'method is deleted'.
    // To avoid checking the format twice, we just check that the format is
    // constexpr. If is it valid, then the overload below will kick in.
    // We add the template here to make this overload have lower priority.
    template<typename = void>
    format_spec_template (const char *s)  // NOLINT
    __attribute__((
    enable_if(format_internal::ensure_constexpr(s), "constexpr trap"),
    unavailable(
    "Format specified does not match the arguments passed.")));

    template<typename T = void>
    format_spec_template (string_view s)  // NOLINT
    __attribute__((enable_if(format_internal::ensure_constexpr(s),
    "constexpr trap"))) {
        static_assert(sizeof(T *) == 0,
                      "Format specified does not match the arguments passed.");
    }

    // Good format overload.
    format_spec_template (const char *s)  // NOLINT
    __attribute__((enable_if(valid_format_impl<argument_to_conv<Args>()...>(s),
    "bad format trap")))
        : Base(s) { }

    format_spec_template (string_view s)  // NOLINT
    __attribute__((enable_if(valid_format_impl<argument_to_conv<Args>()...>(s),
    "bad format trap")))
        : Base(s) { }

#else  // ABEL_INTERNAL_ENABLE_FORMAT_CHECKER

    format_spec_template(const char* s) : Base(s) {}  // NOLINT
    format_spec_template(string_view s) : Base(s) {}  // NOLINT

#endif  // ABEL_INTERNAL_ENABLE_FORMAT_CHECKER

    template<format_conv... C, typename = typename std::enable_if<
        sizeof...(C) == sizeof...(Args) &&
            all_of(conv_contains(argument_to_conv<Args>(),
                           C)...)>::type>
    format_spec_template (const extended_parsed_format<C...> &pc)  // NOLINT
        : Base(&pc) { }
};

template<typename... Args>
struct format_spec_deduction_barrier {
    using type = format_spec_template<Args...>;
};

class stream_able {
public:
    stream_able (const untyped_format_spec_impl &format,
                abel::Span<const format_arg_impl> args)
        : format_(format) {
        if (args.size() <= ABEL_ARRAYSIZE(few_args_)) {
            for (size_t i = 0; i < args.size(); ++i) {
                few_args_[i] = args[i];
            }
            args_ = abel::MakeSpan(few_args_, args.size());
        } else {
            many_args_.assign(args.begin(), args.end());
            args_ = many_args_;
        }
    }

    std::ostream &Print (std::ostream &os) const;

    friend std::ostream &operator << (std::ostream &os, const stream_able &l) {
        return l.Print(os);
    }

private:
    const untyped_format_spec_impl &format_;
    abel::Span<const format_arg_impl> args_;
    // if args_.size() is 4 or less:
    format_arg_impl few_args_[4] = {format_arg_impl(0), format_arg_impl(0),
                                  format_arg_impl(0), format_arg_impl(0)};
    // if args_.size() is more than 4:
    std::vector<format_arg_impl> many_args_;
};

// for testing
std::string Summarize (untyped_format_spec_impl format,
                       abel::Span<const format_arg_impl> args);
bool BindWithPack (const unbound_conversion *props,
                   abel::Span<const format_arg_impl> pack, bound_conversion *bound);

bool FormatUntyped (format_raw_sink_impl raw_sink,
                    untyped_format_spec_impl format,
                    abel::Span<const format_arg_impl> args);

std::string &AppendPack (std::string *out, untyped_format_spec_impl format,
                         abel::Span<const format_arg_impl> args);

std::string FormatPack (const untyped_format_spec_impl format,
                        abel::Span<const format_arg_impl> args);

int FprintF (std::FILE *output, untyped_format_spec_impl format,
             abel::Span<const format_arg_impl> args);
int SnprintF (char *output, size_t size, untyped_format_spec_impl format,
              abel::Span<const format_arg_impl> args);

// Returned by Streamed(v). Converts via '%s' to the std::string created
// by std::ostream << v.
template<typename T>
class streamed_wrapper {
public:
    explicit streamed_wrapper (const T &v) : v_(v) { }

private:
    template<typename S>
    friend convert_result<format_conv::s> format_convert_impl (const streamed_wrapper<S> &v,
                                                     conversion_spec conv,
                                                     format_sink_impl *out);
    const T &v_;
};

}  // namespace format_internal

}  // namespace abel

#endif  // ABEL_FORMAT_INTERNAL_BIND_H_
