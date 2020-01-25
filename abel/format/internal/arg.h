#ifndef ABEL_FORMAT_INTERNAL_ARG_H_
#define ABEL_FORMAT_INTERNAL_ARG_H_

#include <cstring>
#include <cwchar>
#include <cstdio>
#include <iomanip>
#include <limits>
#include <memory>
#include <sstream>
#include <string>
#include <type_traits>
#include <abel/base/profile.h>
#include <abel/meta/type_traits.h>
#include <abel/numeric/int128.h>
#include <abel/format/internal/format_conv.h>
#include <abel/strings/string_view.h>
#include <abel/format/internal/sink_impl.h>
#include <abel/format/internal/conversion_spec.h>
namespace abel {

class Cord;
class format_count_capture;
class FormatSink;

namespace format_internal {

template<typename T, typename = void>
struct has_user_defined_convert : std::false_type { };

template<typename T>
struct has_user_defined_convert<
    T, void_t<decltype(AbelFormatConvert(
        std::declval<const T &>(), std::declval<conversion_spec>(),
        std::declval<FormatSink *>()))>> : std::true_type {
};

template<typename T>
class streamed_wrapper;

// If 'v' can be converted (in the printf sense) according to 'conv',
// then convert it, appending to `sink` and return `true`.
// Otherwise fail and return `false`.

// Raw pointers.
struct void_ptr {
    void_ptr () = default;
    template<typename T,
        decltype(reinterpret_cast<uintptr_t>(std::declval<T *>())) = 0>
    void_ptr (T *ptr)  // NOLINT
        : value(ptr ? reinterpret_cast<uintptr_t>(ptr) : 0) { }
    uintptr_t value;
};

convert_result<format_conv::p> format_convert_impl (void_ptr v, conversion_spec conv,
                                                    format_sink_impl *sink);

// Strings.
convert_result<format_conv::s> format_convert_impl (const std::string &v,
                                                    conversion_spec conv,
                                                    format_sink_impl *sink);
convert_result<format_conv::s> format_convert_impl (string_view v, conversion_spec conv,
                                                    format_sink_impl *sink);
convert_result<format_conv::s | format_conv::p> format_convert_impl (const char *v,
                                                                     conversion_spec conv,
                                                                     format_sink_impl *sink);
template<class AbelCord,
    typename std::enable_if<
        std::is_same<AbelCord, abel::Cord>::value>::type * = nullptr>
convert_result<format_conv::s> format_convert_impl (const AbelCord &value,
                                                    conversion_spec conv,
                                                    format_sink_impl *sink) {
    if (conv.conv().id() != conversion_char::s)
        return {false};

    bool is_left = conv.flags().left;
    size_t space_remaining = 0;

    int width = conv.width();
    if (width >= 0)
        space_remaining = width;

    size_t to_write = value.size();

    int precision = conv.precision();
    if (precision >= 0)
        to_write = (std::min)(to_write, static_cast<size_t>(precision));

    space_remaining = Excess(to_write, space_remaining);

    if (space_remaining > 0 && !is_left)
        sink->Append(space_remaining, ' ');

    for (string_view piece : value.Chunks()) {
        if (piece.size() > to_write) {
            piece.remove_suffix(piece.size() - to_write);
            to_write = 0;
        } else {
            to_write -= piece.size();
        }
        sink->Append(piece);
        if (to_write == 0) {
            break;
        }
    }

    if (space_remaining > 0 && is_left)
        sink->Append(space_remaining, ' ');
    return {true};
}

using integral_convert_result =
convert_result<format_conv::c | format_conv::numeric | format_conv::star>;
using floating_convert_result = convert_result<format_conv::floating>;

// Floats.
floating_convert_result format_convert_impl (float v, conversion_spec conv,
                                             format_sink_impl *sink);
floating_convert_result format_convert_impl (double v, conversion_spec conv,
                                             format_sink_impl *sink);
floating_convert_result format_convert_impl (long double v, conversion_spec conv,
                                             format_sink_impl *sink);

// Chars.
integral_convert_result format_convert_impl (char v, conversion_spec conv,
                                             format_sink_impl *sink);
integral_convert_result format_convert_impl (signed char v, conversion_spec conv,
                                             format_sink_impl *sink);
integral_convert_result format_convert_impl (unsigned char v, conversion_spec conv,
                                             format_sink_impl *sink);

// Ints.
integral_convert_result format_convert_impl (short v,  // NOLINT
                                             conversion_spec conv,
                                             format_sink_impl *sink);
integral_convert_result format_convert_impl (unsigned short v,  // NOLINT
                                             conversion_spec conv,
                                             format_sink_impl *sink);
integral_convert_result format_convert_impl (int v, conversion_spec conv,
                                             format_sink_impl *sink);
integral_convert_result format_convert_impl (unsigned v, conversion_spec conv,
                                             format_sink_impl *sink);
integral_convert_result format_convert_impl (long v,  // NOLINT
                                             conversion_spec conv,
                                             format_sink_impl *sink);
integral_convert_result format_convert_impl (unsigned long v,  // NOLINT
                                             conversion_spec conv,
                                             format_sink_impl *sink);
integral_convert_result format_convert_impl (long long v,  // NOLINT
                                             conversion_spec conv,
                                             format_sink_impl *sink);
integral_convert_result format_convert_impl (unsigned long long v,  // NOLINT
                                             conversion_spec conv,
                                             format_sink_impl *sink);
integral_convert_result format_convert_impl (int128 v, conversion_spec conv,
                                             format_sink_impl *sink);
integral_convert_result format_convert_impl (uint128 v, conversion_spec conv,
                                             format_sink_impl *sink);
template<typename T, enable_if_t<std::is_same<T, bool>::value, int> = 0>
integral_convert_result format_convert_impl (T v, conversion_spec conv,
                                             format_sink_impl *sink) {
    return format_convert_impl(static_cast<int>(v), conv, sink);
}

// We provide this function to help the checker, but it is never defined.
// format_arg_impl will use the underlying Convert functions instead.
template<typename T>
typename std::enable_if<std::is_enum<T>::value &&
    !has_user_defined_convert<T>::value,
                        integral_convert_result>::type
format_convert_impl (T v, conversion_spec conv, format_sink_impl *sink);

template<typename T>
convert_result<format_conv::s> format_convert_impl (const streamed_wrapper<T> &v,
                                                    conversion_spec conv,
                                                    format_sink_impl *out) {
    std::ostringstream oss;
    oss << v.v_;
    if (!oss)
        return {false};
    return format_internal::format_convert_impl(oss.str(), conv, out);
}

// Use templates and dependent types to delay evaluation of the function
// until after format_count_capture is fully defined.
struct format_count_capture_helper {
    template<class T = int>
    static convert_result<format_conv::n> ConvertHelper (const format_count_capture &v,
                                                         conversion_spec conv,
                                                         format_sink_impl *sink) {
        const abel::enable_if_t<sizeof(T) != 0, format_count_capture> &v2 = v;

        if (conv.conv().id() != format_internal::conversion_char::n)
            return {false};
        *v2.p_ = static_cast<int>(sink->size());
        return {true};
    }
};

template<class T = int>
convert_result<format_conv::n> format_convert_impl (const format_count_capture &v,
                                                    conversion_spec conv,
                                                    format_sink_impl *sink) {
    return format_count_capture_helper::ConvertHelper(v, conv, sink);
}

// Helper friend struct to hide implementation details from the public API of
// format_arg_impl.
struct format_arg_impl_friend {
    template<typename Arg>
    static bool to_int (Arg arg, int *out) {
        // A value initialized conversion_spec has a `none` conv, which tells the
        // dispatcher to run the `int` conversion.
        return arg._dispatcher(arg._data, {}, out);
    }

    template<typename Arg>
    static bool convert (Arg arg, format_internal::conversion_spec conv,
                         format_sink_impl *out) {
        return arg._dispatcher(arg._data, conv, out);
    }

    template<typename Arg>
    static typename Arg::dispatcher GetVTablePtrForTest (Arg arg) {
        return arg._dispatcher;
    }
};

// A type-erased handle to a format argument.
class format_arg_impl {
private:
    enum { kInlinedSpace = 8 };

    using void_ptr = format_internal::void_ptr;

    union Data {
        const void *ptr;
        const volatile void *volatile_ptr;
        char buf[kInlinedSpace];
    };

    using dispatcher = bool (*) (Data, conversion_spec, void *out);

    template<typename T>
    struct store_by_value
        : std::integral_constant<bool, (sizeof(T) <= kInlinedSpace) &&
            (std::is_integral<T>::value ||
                std::is_floating_point<T>::value ||
                std::is_pointer<T>::value ||
                std::is_same<void_ptr, T>::value)> {
    };

    enum enum_storage_policy { ByPointer, ByVolatilePointer, ByValue };
    template<typename T>
    struct storage_policy
        : std::integral_constant<enum_storage_policy,
                                 (std::is_volatile<T>::value
                                  ? ByVolatilePointer
                                  : (store_by_value<T>::value ? ByValue
                                                              : ByPointer))> {
    };

    // To reduce the number of vtables we will decay values before hand.
    // Anything with a user-defined Convert will get its own vtable.
    // For everything else:
    //   - Decay char* and char arrays into `const char*`
    //   - Decay any other pointer to `const void*`
    //   - Decay all enums to their underlying type.
    //   - Decay function pointers to void*.
    template<typename T, typename = void>
    struct decay_type {
        static constexpr bool kHasUserDefined =
            format_internal::has_user_defined_convert<T>::value;
        using type = typename std::conditional<
            !kHasUserDefined && std::is_convertible<T, const char *>::value,
            const char *,
            typename std::conditional<!kHasUserDefined &&
                std::is_convertible<T, void_ptr>::value,
                                      void_ptr, const T &>::type>::type;
    };
    template<typename T>
    struct decay_type<T,
                      typename std::enable_if<
                          !format_internal::has_user_defined_convert<T>::value &&
                              std::is_enum<T>::value>::type> {
        using type = typename std::underlying_type<T>::type;
    };

public:
    template<typename T>
    explicit format_arg_impl (const T &value) {
        using D = typename decay_type<T>::type;
        static_assert(
            std::is_same<D, const T &>::value || storage_policy<D>::value == ByValue,
            "Decayed types must be stored by value");
        Init(static_cast<D>(value));
    }

private:
    friend struct format_internal::format_arg_impl_friend;
    template<typename T, enum_storage_policy = storage_policy<T>::value>
    struct manager;

    template<typename T>
    struct manager<T, ByPointer> {
        static Data SetValue (const T &value) {
            Data data;
            data.ptr = std::addressof(value);
            return data;
        }

        static const T &Value (Data arg) { return *static_cast<const T *>(arg.ptr); }
    };

    template<typename T>
    struct manager<T, ByVolatilePointer> {
        static Data SetValue (const T &value) {
            Data data;
            data.volatile_ptr = &value;
            return data;
        }

        static const T &Value (Data arg) {
            return *static_cast<const T *>(arg.volatile_ptr);
        }
    };

    template<typename T>
    struct manager<T, ByValue> {
        static Data SetValue (const T &value) {
            Data data;
            memcpy(data.buf, &value, sizeof(value));
            return data;
        }

        static T Value (Data arg) {
            T value;
            memcpy(&value, arg.buf, sizeof(T));
            return value;
        }
    };

    template<typename T>
    void Init (const T &value) {
        _data = manager<T>::SetValue(value);
        _dispatcher = &Dispatch<T>;
    }

    template<typename T>
    static int ToIntVal (const T &val) {
        using CommonType = typename std::conditional<std::is_signed<T>::value,
                                                     int64_t, uint64_t>::type;
        if (static_cast<CommonType>(val) >
            static_cast<CommonType>((std::numeric_limits<int>::max)())) {
            return (std::numeric_limits<int>::max)();
        } else if (std::is_signed<T>::value &&
            static_cast<CommonType>(val) <
                static_cast<CommonType>((std::numeric_limits<int>::min)())) {
            return (std::numeric_limits<int>::min)();
        }
        return static_cast<int>(val);
    }

    template<typename T>
    static bool ToInt (Data arg, int *out, std::true_type /* is_integral */,
                       std::false_type) {
        *out = ToIntVal(manager<T>::Value(arg));
        return true;
    }

    template<typename T>
    static bool ToInt (Data arg, int *out, std::false_type,
                       std::true_type /* is_enum */) {
        *out = ToIntVal(static_cast<typename std::underlying_type<T>::type>(
                            manager<T>::Value(arg)));
        return true;
    }

    template<typename T>
    static bool ToInt (Data, int *, std::false_type, std::false_type) {
        return false;
    }

    template<typename T>
    static bool Dispatch (Data arg, conversion_spec spec, void *out) {
        // A `none` conv indicates that we want the `int` conversion.
        if (ABEL_UNLIKELY(spec.conv().id() == conversion_char::none)) {
            return ToInt<T>(arg, static_cast<int *>(out), std::is_integral<T>(),
                            std::is_enum<T>());
        }

        return format_internal::format_convert_impl(
            manager<T>::Value(arg), spec, static_cast<format_sink_impl *>(out))
            .value;
    }

    Data _data;
    dispatcher _dispatcher;
};

#define ABEL_INTERNAL_FORMAT_DISPATCH_INSTANTIATE_(T, E) \
  E template bool format_arg_impl::Dispatch<T>(Data, conversion_spec, void*)

#define ABEL_INTERNAL_FORMAT_DISPATCH_OVERLOADS_EXPAND_(...)                   \
  ABEL_INTERNAL_FORMAT_DISPATCH_INSTANTIATE_(format_internal::void_ptr,     \
                                             __VA_ARGS__);                     \
  ABEL_INTERNAL_FORMAT_DISPATCH_INSTANTIATE_(bool, __VA_ARGS__);               \
  ABEL_INTERNAL_FORMAT_DISPATCH_INSTANTIATE_(char, __VA_ARGS__);               \
  ABEL_INTERNAL_FORMAT_DISPATCH_INSTANTIATE_(signed char, __VA_ARGS__);        \
  ABEL_INTERNAL_FORMAT_DISPATCH_INSTANTIATE_(unsigned char, __VA_ARGS__);      \
  ABEL_INTERNAL_FORMAT_DISPATCH_INSTANTIATE_(short, __VA_ARGS__); /* NOLINT */ \
  ABEL_INTERNAL_FORMAT_DISPATCH_INSTANTIATE_(unsigned short,      /* NOLINT */ \
                                             __VA_ARGS__);                     \
  ABEL_INTERNAL_FORMAT_DISPATCH_INSTANTIATE_(int, __VA_ARGS__);                \
  ABEL_INTERNAL_FORMAT_DISPATCH_INSTANTIATE_(unsigned int, __VA_ARGS__);       \
  ABEL_INTERNAL_FORMAT_DISPATCH_INSTANTIATE_(long, __VA_ARGS__); /* NOLINT */  \
  ABEL_INTERNAL_FORMAT_DISPATCH_INSTANTIATE_(unsigned long,      /* NOLINT */  \
                                             __VA_ARGS__);                     \
  ABEL_INTERNAL_FORMAT_DISPATCH_INSTANTIATE_(long long, /* NOLINT */           \
                                             __VA_ARGS__);                     \
  ABEL_INTERNAL_FORMAT_DISPATCH_INSTANTIATE_(unsigned long long, /* NOLINT */  \
                                             __VA_ARGS__);                     \
  ABEL_INTERNAL_FORMAT_DISPATCH_INSTANTIATE_(int128, __VA_ARGS__);             \
  ABEL_INTERNAL_FORMAT_DISPATCH_INSTANTIATE_(uint128, __VA_ARGS__);            \
  ABEL_INTERNAL_FORMAT_DISPATCH_INSTANTIATE_(float, __VA_ARGS__);              \
  ABEL_INTERNAL_FORMAT_DISPATCH_INSTANTIATE_(double, __VA_ARGS__);             \
  ABEL_INTERNAL_FORMAT_DISPATCH_INSTANTIATE_(long double, __VA_ARGS__);        \
  ABEL_INTERNAL_FORMAT_DISPATCH_INSTANTIATE_(const char*, __VA_ARGS__);        \
  ABEL_INTERNAL_FORMAT_DISPATCH_INSTANTIATE_(std::string, __VA_ARGS__);        \
  ABEL_INTERNAL_FORMAT_DISPATCH_INSTANTIATE_(string_view, __VA_ARGS__)

ABEL_INTERNAL_FORMAT_DISPATCH_OVERLOADS_EXPAND_(extern);

}  // namespace format_internal

}  // namespace abel

#endif  // ABEL_FORMAT_INTERNAL_ARG_H_
