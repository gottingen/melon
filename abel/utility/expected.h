//
// Created by liyinbin on 2021/5/1.
//

#ifndef ABEL_UTILITY_EXPECTED_H_
#define ABEL_UTILITY_EXPECTED_H_

#include <optional>
#include <type_traits>
#include <utility>
#include <variant>

namespace abel {

    template<class T, class E = void>
    class expected {
    public:
        constexpr expected() = default;

        template<class U, class = std::enable_if_t<std::is_constructible_v<T, U> &&
                                                   std::is_convertible_v<U &&, T>>>
        constexpr /* implicit */ expected(U &&value)
                : _value(std::forward<U>(value)) {}

        constexpr /* implicit */ expected(E error)
                : _value(std::in_place_index<1>, std::move(error)) {}

        constexpr T *operator->() { return &value(); }

        constexpr const T *operator->() const { return &value(); }

        constexpr T &operator*() { return value(); }

        constexpr const T &operator*() const { return value(); }

        constexpr explicit operator bool() const noexcept {
            return _value.index() == 0;
        }

        constexpr T &value() { return std::get<0>(_value); }

        constexpr const T &value() const { return std::get<0>(_value); }

        constexpr E &error() { return std::get<1>(_value); }

        constexpr const E &error() const { return std::get<1>(_value); }

        template<class U>
        constexpr T value_or(U &&alternative) const {
            if (*this) {
                return value();
            } else {
                return std::forward<U>(alternative);
            }
        }

    private:
        std::variant<T, E> _value;
    };

    template<class E>
    class expected<void, E> {
    public:
        constexpr expected() = default;

        constexpr /* implicit */ expected(E error) : _error(std::move(error)) {}

        constexpr explicit operator bool() const noexcept { return !_error; }

        constexpr E &error() { return *_error; }

        constexpr const E &error() const { return *_error; }

    private:
        std::optional<E> _error;
    };

}  // namespace abel

#endif  // ABEL_UTILITY_EXPECTED_H_
