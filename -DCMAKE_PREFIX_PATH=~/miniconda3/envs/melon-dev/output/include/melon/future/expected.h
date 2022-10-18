

/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/

#ifndef MELON_FUTURE_EXPECTED_H_
#define MELON_FUTURE_EXPECTED_H_

#include <utility>
#include <cassert>
#include <exception>
#include <functional>
#include <initializer_list>
#include <new>
#include <stdexcept>
#include <system_error>
#include <type_traits>
#include "melon/base/type_traits.h"

#define ENABLE_IF_REQUIRES_0(...) \
    template< bool B = (__VA_ARGS__), typename std::enable_if<B, int>::type = 0 >

#define ENABLE_IF_REQUIRES_T(...) \
    , typename = typename std::enable_if< (__VA_ARGS__), melon::expected_lite::detail::enabler >::type

#define ENABLE_IF_REQUIRES_R(R, ...) \
    typename std::enable_if< (__VA_ARGS__), R>::type

#define ENABLE_IF_REQUIRES_A(...) \
    , typename std::enable_if< (__VA_ARGS__), void*>::type = nullptr

namespace melon {
    namespace expected_lite {

        // forward declaration:

        template<typename T, typename E>
        class expected;

        namespace detail {

            /// for ENABLE_IF_REQUIRES_T

            enum class enabler {
            };

            /// discriminated union to hold value or 'error'.

            template<typename T, typename E>
            union storage_t {
                template<typename, typename> friend
                class melon::expected_lite::expected;

            private:
                using value_type = T;
                using error_type = E;

                // no-op construction
                storage_t() {}

                ~storage_t() {}

                void construct_value(value_type const &e) {
                    new(&m_value) value_type(e);
                }

                void construct_value(value_type &&e) {
                    new(&m_value) value_type(std::move(e));
                }

                template<class... Args>
                void emplace_value(Args &&... args) {
                    new(&m_value) value_type(std::forward<Args>(args)...);
                }

                template<class U, class... Args>
                void emplace_value(std::initializer_list<U> il, Args &&... args) {
                    new(&m_value) value_type(il, std::forward<Args>(args)...);
                }

                void destruct_value() {
                    m_value.~value_type();
                }

                void construct_error(error_type const &e) {
                    new(&m_error) error_type(e);
                }

                void construct_error(error_type &&e) {
                    new(&m_error) error_type(std::move(e));
                }

                template<class... Args>
                void emplace_error(Args &&... args) {
                    new(&m_error) error_type(std::forward<Args>(args)...);
                }

                template<class U, class... Args>
                void emplace_error(std::initializer_list<U> il, Args &&... args) {
                    new(&m_error) error_type(il, std::forward<Args>(args)...);
                }

                void destruct_error() {
                    m_error.~error_type();
                }

                constexpr value_type const &value() const &{
                    return m_value;
                }

                value_type &value() &{
                    return m_value;
                }

                constexpr value_type const &&value() const &&{
                    return std::move(m_value);
                }

                constexpr value_type &&value() &&{
                    return std::move(m_value);
                }

                value_type const *value_ptr() const {
                    return &m_value;
                }

                value_type *value_ptr() {
                    return &m_value;
                }

                error_type const &error() const &{
                    return m_error;
                }

                error_type &error() &{
                    return m_error;
                }

                constexpr error_type const &&error() const &&{
                    return std::move(m_error);
                }

                constexpr error_type &&error() &&{
                    return std::move(m_error);
                }

            private:
                value_type m_value;
                error_type m_error;
            };

            /// discriminated union to hold only 'error'.

            template<typename E>
            union storage_t<void, E> {
                template<typename, typename> friend
                class melon::expected_lite::expected;

            private:
                using value_type = void;
                using error_type = E;

                // no-op construction
                storage_t() {}

                ~storage_t() {}

                void construct_error(error_type const &e) {
                    new(&m_error) error_type(e);
                }

                void construct_error(error_type &&e) {
                    new(&m_error) error_type(std::move(e));
                }

                template<class... Args>
                void emplace_error(Args &&... args) {
                    new(&m_error) error_type(std::forward<Args>(args)...);
                }

                template<class U, class... Args>
                void emplace_error(std::initializer_list<U> il, Args &&... args) {
                    new(&m_error) error_type(il, std::forward<Args>(args)...);
                }

                void destruct_error() {
                    m_error.~error_type();
                }

                error_type const &error() const &{
                    return m_error;
                }

                error_type &error() &{
                    return m_error;
                }

                constexpr error_type const &&error() const &&{
                    return std::move(m_error);
                }

                constexpr error_type &&error() &&{
                    return std::move(m_error);
                }

            private:
                error_type m_error;
            };

        } // namespace detail

        /// x.x.5 Unexpected object type; unexpected_type; C++17 and later can also use aliased type unexpected.

        template<typename E>
        class unexpected_type {
        public:
            using error_type = E;

            // x.x.5.2.1 Constructors

            //  unexpected_type() = delete;

            constexpr unexpected_type(unexpected_type const &) = default;

            constexpr unexpected_type(unexpected_type &&) = default;

            template<typename... Args ENABLE_IF_REQUIRES_T(
                    std::is_constructible<E, Args && ...>::value
            )
            >
            constexpr explicit unexpected_type(std::in_place_t, Args &&... args)
                    : m_error(std::forward<Args>(args)...) {}

            template<typename U, typename... Args ENABLE_IF_REQUIRES_T(
                    std::is_constructible<E, std::initializer_list<U>, Args && ...>::value
            )
            >
            constexpr explicit unexpected_type(std::in_place_t, std::initializer_list<U> il, Args &&... args)
                    : m_error(il, std::forward<Args>(args)...) {}

            template<typename E2 ENABLE_IF_REQUIRES_T(
                    std::is_constructible<E, E2>::value
                    && !std::is_same<typename melon::remove_cvref<E2>::type, std::in_place_t>::value
                    && !std::is_same<typename melon::remove_cvref<E2>::type, unexpected_type>::value
            )
            >
            constexpr explicit unexpected_type(E2 &&error)
                    : m_error(std::forward<E2>(error)) {}

            template<typename E2>
            constexpr explicit unexpected_type(unexpected_type<E2> const &error ENABLE_IF_REQUIRES_A(
                    std::is_constructible<E, E2>::value
                    && !std::is_constructible<E, unexpected_type<E2> &>::value
                    && !std::is_constructible<E, unexpected_type<E2> >::value
                    && !std::is_constructible<E, unexpected_type<E2> const &>::value
                    && !std::is_constructible<E, unexpected_type<E2> const>::value
                    && !std::is_convertible<unexpected_type<E2> &, E>::value
                    && !std::is_convertible<unexpected_type<E2>, E>::value
                    && !std::is_convertible<unexpected_type<E2> const &, E>::value
                    && !std::is_convertible<unexpected_type<E2> const, E>::value
                    && !std::is_convertible<E2 const &, E>::value /*=> explicit */ )
            )
                    : m_error(E{error.value()}) {}

            template<typename E2>
            constexpr /*non-explicit*/ unexpected_type(unexpected_type<E2> const &error ENABLE_IF_REQUIRES_A(
                    std::is_constructible<E, E2>::value
                    && !std::is_constructible<E, unexpected_type<E2> &>::value
                    && !std::is_constructible<E, unexpected_type<E2> >::value
                    && !std::is_constructible<E, unexpected_type<E2> const &>::value
                    && !std::is_constructible<E, unexpected_type<E2> const>::value
                    && !std::is_convertible<unexpected_type<E2> &, E>::value
                    && !std::is_convertible<unexpected_type<E2>, E>::value
                    && !std::is_convertible<unexpected_type<E2> const &, E>::value
                    && !std::is_convertible<unexpected_type<E2> const, E>::value
                    && std::is_convertible<E2 const &, E>::value /*=> explicit */ )
            )
                    : m_error(error.value()) {}

            template<typename E2>
            constexpr explicit unexpected_type(unexpected_type<E2> &&error ENABLE_IF_REQUIRES_A(
                    std::is_constructible<E, E2>::value
                    && !std::is_constructible<E, unexpected_type<E2> &>::value
                    && !std::is_constructible<E, unexpected_type<E2> >::value
                    && !std::is_constructible<E, unexpected_type<E2> const &>::value
                    && !std::is_constructible<E, unexpected_type<E2> const>::value
                    && !std::is_convertible<unexpected_type<E2> &, E>::value
                    && !std::is_convertible<unexpected_type<E2>, E>::value
                    && !std::is_convertible<unexpected_type<E2> const &, E>::value
                    && !std::is_convertible<unexpected_type<E2> const, E>::value
                    && !std::is_convertible<E2 const &, E>::value /*=> explicit */ )
            )
                    : m_error(E{std::move(error.value())}) {}

            template<typename E2>
            constexpr /*non-explicit*/ unexpected_type(unexpected_type<E2> &&error ENABLE_IF_REQUIRES_A(
                    std::is_constructible<E, E2>::value
                    && !std::is_constructible<E, unexpected_type<E2> &>::value
                    && !std::is_constructible<E, unexpected_type<E2> >::value
                    && !std::is_constructible<E, unexpected_type<E2> const &>::value
                    && !std::is_constructible<E, unexpected_type<E2> const>::value
                    && !std::is_convertible<unexpected_type<E2> &, E>::value
                    && !std::is_convertible<unexpected_type<E2>, E>::value
                    && !std::is_convertible<unexpected_type<E2> const &, E>::value
                    && !std::is_convertible<unexpected_type<E2> const, E>::value
                    && std::is_convertible<E2 const &, E>::value /*=> non-explicit */ )
            )
                    : m_error(std::move(error.value())) {}

            // x.x.5.2.2 Assignment

            constexpr unexpected_type &operator=(unexpected_type const &) = default;

            constexpr unexpected_type &operator=(unexpected_type &&) = default;

            template<typename E2 = E>
            constexpr unexpected_type &operator=(unexpected_type<E2> const &other) {
                unexpected_type{other.value()}.swap(*this);
                return *this;
            }

            template<typename E2 = E>
            constexpr unexpected_type &operator=(unexpected_type<E2> &&other) {
                unexpected_type{std::move(other.value())}.swap(*this);
                return *this;
            }

            // x.x.5.2.3 Observers

            constexpr E &value() & noexcept {
                return m_error;
            }

            constexpr E const &value() const & noexcept {
                return m_error;
            }

            constexpr E &&value() && noexcept {
                return std::move(m_error);
            }

            constexpr E const &&value() const && noexcept {
                return std::move(m_error);
            }


            // x.x.5.2.4 Swap

            ENABLE_IF_REQUIRES_R(void,
                            std::is_swappable<E>::value
            )
            swap(unexpected_type &other) noexcept(
            std::is_nothrow_swappable<E>::value
            ) {
                using std::swap;
                swap(m_error, other.m_error);
            }

            // TODO: ??? unexpected_type: in-class friend operator==, !=

        private:
            error_type m_error;
        };

        /// template deduction guide:

        template<typename E>
        unexpected_type(E) -> unexpected_type<E>;


        /// class unexpected_type, std::exception_ptr specialization (P0323R2)

        /// x.x.4, Unexpected equality operators

        template<typename E1, typename E2>
        constexpr bool operator==(unexpected_type<E1> const &x, unexpected_type<E2> const &y) {
            return x.value() == y.value();
        }

        template<typename E1, typename E2>
        constexpr bool operator!=(unexpected_type<E1> const &x, unexpected_type<E2> const &y) {
            return !(x == y);
        }

        /// x.x.5 Specialized algorithms

        template<typename E ENABLE_IF_REQUIRES_T(
                std::is_swappable<E>::value
        )
        >
        void swap(unexpected_type<E> &x, unexpected_type<E> &y) noexcept(noexcept(x.swap(y))) {
            x.swap(y);
        }

        template<typename E>
        constexpr auto
        make_unexpected(E &&value) -> unexpected_type<typename std::decay<E>::type> {
            return unexpected_type<typename std::decay<E>::type>(std::forward<E>(value));
        }


        /// x.x.6, x.x.7 expected access error

        template<typename E>
        class bad_expected_access;

        /// x.x.7 bad_expected_access<void>: expected access error

        template<>
        class bad_expected_access<void> : public std::exception {
        public:
            explicit bad_expected_access()
                    : std::exception() {}
        };

        /// x.x.6 bad_expected_access: expected access error

        template<typename E>
        class bad_expected_access : public bad_expected_access<void> {
        public:
            using error_type = E;

            explicit bad_expected_access(error_type error)
                    : m_error(error) {}

            virtual char const *what() const noexcept override {
                return "bad_expected_access";
            }

            constexpr error_type &error() &{
                return m_error;
            }

            constexpr error_type const &error() const &{
                return m_error;
            }

            constexpr error_type &&error() &&{
                return std::move(m_error);
            }

            constexpr error_type const &&error() const &&{
                return std::move(m_error);
            }

        private:
            error_type m_error;
        };

        /// x.x.8 unexpect tag, in_place_unexpected tag: construct an error

        struct unexpect_t {
        };
        using in_place_unexpected_t = unexpect_t;

        inline constexpr unexpect_t unexpect{};
        inline constexpr unexpect_t in_place_unexpected{};

        /// class error_traits

        template<typename Error>
        struct error_traits {
            static void rethrow(Error const &e) {
                throw bad_expected_access<Error>{e};
            }
        };

        template<>
        struct error_traits<std::exception_ptr> {
            static void rethrow(std::exception_ptr const &e) {
                std::rethrow_exception(e);
            }
        };

        template<>
        struct error_traits<std::error_code> {
            static void rethrow(std::error_code const &e) {
                throw std::system_error(e);
            }
        };

    } // namespace expected_lite

    // provide melon::unexpected_type:

    using expected_lite::unexpected_type;

    namespace expected_lite {

        /// class expected

        template<typename T, typename E = std::exception_ptr>
        class expected {
        private:
            template<typename, typename> friend
            class expected;


        public:
            using value_type = T;
            using error_type = E;
            using unexpected_type = melon::unexpected_type<E>;

            template<typename U>
            struct rebind {
                using type = expected<U, error_type>;
            };

            // x.x.4.1 constructors

            ENABLE_IF_REQUIRES_0(
                    std::is_default_constructible<T>::value
            )
            constexpr expected()
                    : has_value_(true) {
                contained.construct_value(value_type());
            }

            constexpr expected(expected const &other ENABLE_IF_REQUIRES_A(
                    std::is_copy_constructible<T>::value
                    && std::is_copy_constructible<E>::value
            )
            )
                    : has_value_(other.has_value_) {
                if (has_value()) contained.construct_value(other.contained.value());
                else contained.construct_error(other.contained.error());
            }

            constexpr expected(expected &&other ENABLE_IF_REQUIRES_A(
                    std::is_move_constructible<T>::value
                    && std::is_move_constructible<E>::value
            )
            ) noexcept(
            std::is_nothrow_move_constructible<T>::value
            && std::is_nothrow_move_constructible<E>::value
            )
                    : has_value_(other.has_value_) {
                if (has_value()) contained.construct_value(std::move(other.contained.value()));
                else contained.construct_error(std::move(other.contained.error()));
            }

            template<typename U, typename G>
            constexpr explicit expected(expected<U, G> const &other ENABLE_IF_REQUIRES_A(
                    std::is_constructible<T, U const &>::value
                    && std::is_constructible<E, G const &>::value
                    && !std::is_constructible<T, expected<U, G> &>::value
                    && !std::is_constructible<T, expected<U, G> &&>::value
                    && !std::is_constructible<T, expected<U, G> const &>::value
                    && !std::is_constructible<T, expected<U, G> const &&>::value
                    && !std::is_convertible<expected<U, G> &, T>::value
                    && !std::is_convertible<expected<U, G> &&, T>::value
                    && !std::is_convertible<expected<U, G> const &, T>::value
                    && !std::is_convertible<expected<U, G> const &&, T>::value
                    && (!std::is_convertible<U const &, T>::value ||
                        !std::is_convertible<G const &, E>::value) /*=> explicit */ )
            )
                    : has_value_(other.has_value_) {
                if (has_value()) contained.construct_value(T{other.contained.value()});
                else contained.construct_error(E{other.contained.error()});
            }

            template<typename U, typename G>
            constexpr /*non-explicit*/ expected(expected<U, G> const &other ENABLE_IF_REQUIRES_A(
                    std::is_constructible<T, U const &>::value
                    && std::is_constructible<E, G const &>::value
                    && !std::is_constructible<T, expected<U, G> &>::value
                    && !std::is_constructible<T, expected<U, G> &&>::value
                    && !std::is_constructible<T, expected<U, G> const &>::value
                    && !std::is_constructible<T, expected<U, G> const &&>::value
                    && !std::is_convertible<expected<U, G> &, T>::value
                    && !std::is_convertible<expected<U, G> &&, T>::value
                    && !std::is_convertible<expected<U, G> const &, T>::value
                    && !std::is_convertible<expected<U, G> const &&, T>::value
                    && !(!std::is_convertible<U const &, T>::value ||
                         !std::is_convertible<G const &, E>::value) /*=> non-explicit */ )
            )
                    : has_value_(other.has_value_) {
                if (has_value()) contained.construct_value(other.contained.value());
                else contained.construct_error(other.contained.error());
            }

            template<typename U, typename G>
            constexpr explicit expected(expected<U, G> &&other ENABLE_IF_REQUIRES_A(
                    std::is_constructible<T, U>::value
                    && std::is_constructible<E, G>::value
                    && !std::is_constructible<T, expected<U, G> &>::value
                    && !std::is_constructible<T, expected<U, G> &&>::value
                    && !std::is_constructible<T, expected<U, G> const &>::value
                    && !std::is_constructible<T, expected<U, G> const &&>::value
                    && !std::is_convertible<expected<U, G> &, T>::value
                    && !std::is_convertible<expected<U, G> &&, T>::value
                    && !std::is_convertible<expected<U, G> const &, T>::value
                    && !std::is_convertible<expected<U, G> const &&, T>::value
                    && (!std::is_convertible<U, T>::value || !std::is_convertible<G, E>::value) /*=> explicit */ )
            )
                    : has_value_(other.has_value_) {
                if (has_value()) contained.construct_value(T{std::move(other.contained.value())});
                else contained.construct_error(E{std::move(other.contained.error())});
            }

            template<typename U, typename G>
            constexpr /*non-explicit*/ expected(expected<U, G> &&other ENABLE_IF_REQUIRES_A(
                    std::is_constructible<T, U>::value
                    && std::is_constructible<E, G>::value
                    && !std::is_constructible<T, expected<U, G> &>::value
                    && !std::is_constructible<T, expected<U, G> &&>::value
                    && !std::is_constructible<T, expected<U, G> const &>::value
                    && !std::is_constructible<T, expected<U, G> const &&>::value
                    && !std::is_convertible<expected<U, G> &, T>::value
                    && !std::is_convertible<expected<U, G> &&, T>::value
                    && !std::is_convertible<expected<U, G> const &, T>::value
                    && !std::is_convertible<expected<U, G> const &&, T>::value
                    && !(!std::is_convertible<U, T>::value || !std::is_convertible<G, E>::value) /*=> non-explicit */ )
            )
                    : has_value_(other.has_value_) {
                if (has_value()) contained.construct_value(std::move(other.contained.value()));
                else contained.construct_error(std::move(other.contained.error()));
            }

            constexpr expected(value_type const &value ENABLE_IF_REQUIRES_A(
                    std::is_copy_constructible<T>::value)
            )
                    : has_value_(true) {
                contained.construct_value(value);
            }

            template<typename U = T>
            constexpr explicit expected(U &&value ENABLE_IF_REQUIRES_A(
                    std::is_constructible<T, U &&>::value
                    && !std::is_same<typename melon::remove_cvref<U>::type, std::in_place_t>::value
                    && !std::is_same<expected<T, E>, typename melon::remove_cvref<U>::type>::value
                    && !std::is_same<melon::unexpected_type<E>, typename melon::remove_cvref<U>::type>::value
                    && !std::is_convertible<U &&, T>::value /*=> explicit */
            )
            ) noexcept
            (
            std::is_nothrow_move_constructible<U>::value &&
            std::is_nothrow_move_constructible<E>::value
            )
                    : has_value_(true) {
                contained.construct_value(T{std::forward<U>(value)});
            }

            template<typename U = T>
            constexpr /*non-explicit*/ expected(U &&value ENABLE_IF_REQUIRES_A(
                    std::is_constructible<T, U &&>::value
                    && !std::is_same<typename melon::remove_cvref<U>::type, std::in_place_t>::value
                    && !std::is_same<expected<T, E>, typename melon::remove_cvref<U>::type>::value
                    && !std::is_same<melon::unexpected_type<E>, typename melon::remove_cvref<U>::type>::value
                    && std::is_convertible<U &&, T>::value /*=> non-explicit */
            )
            ) noexcept
            (
            std::is_nothrow_move_constructible<U>::value &&
            std::is_nothrow_move_constructible<E>::value
            )
                    : has_value_(true) {
                contained.construct_value(std::forward<U>(value));
            }

            // construct error:

            template<typename G = E>
            constexpr explicit expected(melon::unexpected_type<G> const &error ENABLE_IF_REQUIRES_A(
                    std::is_constructible<E, G const &>::value
                    && !std::is_convertible<G const &, E>::value /*=> explicit */ )
            )
                    : has_value_(false) {
                contained.construct_error(E{error.value()});
            }

            template<typename G = E>
            constexpr /*non-explicit*/ expected(melon::unexpected_type<G> const &error ENABLE_IF_REQUIRES_A(
                    std::is_constructible<E, G const &>::value
                    && std::is_convertible<G const &, E>::value /*=> non-explicit */ )
            )
                    : has_value_(false) {
                contained.construct_error(error.value());
            }

            template<typename G = E>
            constexpr explicit expected(melon::unexpected_type<G> &&error ENABLE_IF_REQUIRES_A(
                    std::is_constructible<E, G &&>::value
                    && !std::is_convertible<G &&, E>::value /*=> explicit */ )
            )
                    : has_value_(false) {
                contained.construct_error(E{std::move(error.value())});
            }

            template<typename G = E>
            constexpr /*non-explicit*/ expected(melon::unexpected_type<G> &&error ENABLE_IF_REQUIRES_A(
                    std::is_constructible<E, G &&>::value
                    && std::is_convertible<G &&, E>::value /*=> non-explicit */ )
            )
                    : has_value_(false) {
                contained.construct_error(std::move(error.value()));
            }

            // in-place construction, value

            template<typename... Args ENABLE_IF_REQUIRES_T(
                    std::is_constructible<T, Args && ...>::value
            )
            >
            constexpr explicit expected(std::in_place_t, Args &&... args)
                    : has_value_(true) {
                contained.emplace_value(std::forward<Args>(args)...);
            }

            template<typename U, typename... Args ENABLE_IF_REQUIRES_T(
                    std::is_constructible<T, std::initializer_list<U>, Args && ...>::value
            )
            >
            constexpr explicit expected(std::in_place_t, std::initializer_list<U> il, Args &&... args)
                    : has_value_(true) {
                contained.emplace_value(il, std::forward<Args>(args)...);
            }

            // in-place construction, error

            template<typename... Args ENABLE_IF_REQUIRES_T(
                    std::is_constructible<E, Args && ...>::value
            )
            >
            constexpr explicit expected(unexpect_t, Args &&... args)
                    : has_value_(false) {
                contained.emplace_error(std::forward<Args>(args)...);
            }

            template<typename U, typename... Args ENABLE_IF_REQUIRES_T(
                    std::is_constructible<E, std::initializer_list<U>, Args && ...>::value
            )
            >
            constexpr explicit expected(unexpect_t, std::initializer_list<U> il, Args &&... args)
                    : has_value_(false) {
                contained.emplace_error(il, std::forward<Args>(args)...);
            }

            // x.x.4.2 destructor

            // TODO: ~expected: triviality
            // Effects: If T is not cv void and is_trivially_destructible_v<T> is false and bool(*this), calls val.~T(). If is_trivially_destructible_v<E> is false and !bool(*this), calls unexpect.~unexpected<E>().
            // Remarks: If either T is cv void or is_trivially_destructible_v<T> is true, and is_trivially_destructible_v<E> is true, then this destructor shall be a trivial destructor.

            ~expected() {
                if (has_value()) contained.destruct_value();
                else contained.destruct_error();
            }

            // x.x.4.3 assignment

            ENABLE_IF_REQUIRES_R(
                    expected &,
                    std::is_copy_constructible<T>::value
                    && std::is_copy_assignable<T>::value
                    && std::is_copy_constructible<E>::value
                    && std::is_copy_assignable<E>::value
                    && (std::is_nothrow_move_constructible<T>::value
                        || std::is_nothrow_move_constructible<E>::value)
            )
            operator=(expected const &other) {
                expected(other).swap(*this);
                return *this;
            }

            ENABLE_IF_REQUIRES_R(
                    expected &,
                    std::is_move_constructible<T>::value
                    && std::is_move_assignable<T>::value
                    && std::is_move_constructible<E>::value // TODO: std::is_nothrow_move_constructible<E>
                    && std::is_move_assignable<E>::value // TODO: std::is_nothrow_move_assignable<E>
            )
            operator=(expected &&other) noexcept
            (
            std::is_nothrow_move_constructible<T>::value
            && std::is_nothrow_move_assignable<T>::value
            && std::is_nothrow_move_constructible<E>::value     // added for missing
            && std::is_nothrow_move_assignable<E>::value)   //   nothrow above
            {
                expected(std::move(other)).swap(*this);
                return *this;
            }

            template<typename U ENABLE_IF_REQUIRES_T(
                    !std::is_same<expected<T, E>, typename melon::remove_cvref<U>::type>::value
                    && std::conjunction<std::is_scalar<T>, std::is_same<T, std::decay<U>>>::value
                    && std::is_constructible<T, U>::value
                    && std::is_assignable<T &, U>::value
                    && std::is_nothrow_move_constructible<E>::value)
            >
            expected &operator=(U &&value) {
                expected(std::forward<U>(value)).swap(*this);
                return *this;
            }

            template<typename G ENABLE_IF_REQUIRES_T(
                    std::is_copy_constructible<E>::value    // TODO: std::is_nothrow_copy_constructible<E>
                    && std::is_copy_assignable<E>::value
            )
            >
            expected &operator=(melon::unexpected_type<G> const &error) {
                expected(unexpect, error.value()).swap(*this);
                return *this;
            }

            template<typename G ENABLE_IF_REQUIRES_T(
                    std::is_move_constructible<E>::value    // TODO: std::is_nothrow_move_constructible<E>
                    && std::is_move_assignable<E>::value
            )
            >
            expected &operator=(melon::unexpected_type<G> &&error) {
                expected(unexpect, std::move(error.value())).swap(*this);
                return *this;
            }

            template<typename... Args ENABLE_IF_REQUIRES_T(
                    std::is_nothrow_constructible<T, Args && ...>::value
            )
            >
            value_type &emplace(Args &&... args) {
                expected(std::in_place_t{}, std::forward<Args>(args)...).swap(*this);
                return value();
            }

            template<typename U, typename... Args ENABLE_IF_REQUIRES_T(
                    std::is_nothrow_constructible<T, std::initializer_list<U> &, Args && ...>::value
            )
            >
            value_type &emplace(std::initializer_list<U> il, Args &&... args) {
                expected(std::in_place_t{}, il, std::forward<Args>(args)...).swap(*this);
                return value();
            }

            // x.x.4.4 swap

            ENABLE_IF_REQUIRES_R(void,
                            std::is_swappable<T>::value
                            && std::is_swappable<E>::value
                            && (std::is_move_constructible<T>::value || std::is_move_constructible<E>::value)
            )
            swap(expected &other) noexcept
            (
            std::is_nothrow_move_constructible<T>::value && std::is_nothrow_swappable<T &>::value &&
            std::is_nothrow_move_constructible<E>::value && std::is_nothrow_swappable<E &>::value
            ) {
                using std::swap;

                if (bool(*this) && bool(other)) { swap(contained.value(), other.contained.value()); }
                else if (!bool(*this) && !bool(other)) { swap(contained.error(), other.contained.error()); }
                else if (bool(*this) && !bool(other)) {
                    error_type t(std::move(other.error()));
                    other.contained.destruct_error();
                    other.contained.construct_value(std::move(contained.value()));
                    contained.destruct_value();
                    contained.construct_error(std::move(t));
                    swap(has_value_, other.has_value_);
                } else if (!bool(*this) && bool(other)) { other.swap(*this); }
            }

            // x.x.4.5 observers

            constexpr value_type const *operator->() const {
                return assert(has_value()), contained.value_ptr();
            }

            value_type *operator->() {
                return assert(has_value()), contained.value_ptr();
            }

            constexpr value_type const &operator*() const &{
                return assert(has_value()), contained.value();
            }

            value_type &operator*() &{
                return assert(has_value()), contained.value();
            }

            constexpr value_type const &&operator*() const &&{
                return assert(has_value()), std::move(contained.value());
            }

            constexpr value_type &&operator*() &&{
                return assert(has_value()), std::move(contained.value());
            }


            constexpr explicit operator bool() const noexcept {
                return has_value();
            }

            constexpr bool has_value() const noexcept {
                return has_value_;
            }

            constexpr value_type const &value() const &{
                return has_value()
                       ? (contained.value())
                       : (error_traits<error_type>::rethrow(contained.error()), contained.value());
            }

            value_type &value() &{
                return has_value()
                       ? (contained.value())
                       : (error_traits<error_type>::rethrow(contained.error()), contained.value());
            }

            constexpr value_type const &&value() const &&{
                return std::move(has_value()
                                 ? (contained.value())
                                 : (error_traits<error_type>::rethrow(contained.error()), contained.value()));
            }

            constexpr value_type &&value() &&{
                return std::move(has_value()
                                 ? (contained.value())
                                 : (error_traits<error_type>::rethrow(contained.error()), contained.value()));
            }


            constexpr error_type const &error() const &{
                return assert(!has_value()), contained.error();
            }

            error_type &error() &{
                return assert(!has_value()), contained.error();
            }

            constexpr error_type const &&error() const &&{
                return assert(!has_value()), std::move(contained.error());
            }

            error_type &&error() &&{
                return assert(!has_value()), std::move(contained.error());
            }

            constexpr unexpected_type get_unexpected() const {
                return make_unexpected(contained.error());
            }

            template<typename Ex>
            bool has_exception() const {
                using ContainedEx = typename std::remove_reference<decltype(get_unexpected().value())>::type;
                return !has_value() && std::is_base_of<Ex, ContainedEx>::value;
            }

            template<typename U ENABLE_IF_REQUIRES_T(
                    std::is_copy_constructible<T>::value
                    && std::is_convertible<U &&, T>::value
            )
            >
            value_type value_or(U &&v) const &{
                return has_value()
                       ? contained.value()
                       : static_cast<T>( std::forward<U>(v));
            }

            template<typename U ENABLE_IF_REQUIRES_T(
                    std::is_move_constructible<T>::value
                    && std::is_convertible<U &&, T>::value
            )
            >
            value_type value_or(U &&v) &&{
                return has_value()
                       ? std::move(contained.value())
                       : static_cast<T>( std::forward<U>(v));
            }

            // unwrap()

//  template <class U, class E>
//  constexpr expected<U,E> expected<expected<U,E>,E>::unwrap() const&;

//  template <class T, class E>
//  constexpr expected<T,E> expected<T,E>::unwrap() const&;

//  template <class U, class E>
//  expected<U,E> expected<expected<U,E>, E>::unwrap() &&;

//  template <class T, class E>
//  template expected<T,E> expected<T,E>::unwrap() &&;

            // factories

//  template< typename Ex, typename F>
//  expected<T,E> catch_exception(F&& f);

//  template< typename F>
//  expected<decltype(func(declval<T>())),E> map(F&& func) ;

//  template< typename F>
//  'see below' bind(F&& func);

//  template< typename F>
//  expected<T,E> catch_error(F&& f);

//  template< typename F>
//  'see below' then(F&& func);

        private:
            bool has_value_;
            detail::storage_t<T, E> contained;
        };

/// class expected, void specialization

        template<typename E>
        class expected<void, E> {
        private:
            template<typename, typename> friend
            class expected;

        public:
            using value_type = void;
            using error_type = E;
            using unexpected_type = melon::unexpected_type<E>;

            // x.x.4.1 constructors

            constexpr expected() noexcept
                    : has_value_(true) {}

            constexpr expected(expected const &other)
                    : has_value_(other.has_value_) {
                if (!has_value()) {
                    contained.construct_error(other.contained.error());
                }
            }

            ENABLE_IF_REQUIRES_0(
                    std::is_move_constructible<E>::value
            )
            constexpr expected(expected &&other) noexcept
            (
            std::is_nothrow_move_constructible<E>::value
            )
                    : has_value_(other.has_value_) {
                if (!has_value()) {
                    contained.construct_error(std::move(other.contained.error()));
                }

            }

            constexpr explicit expected(std::in_place_t)
                    : has_value_(true) {}

            template<typename G = E>
            constexpr explicit expected(melon::unexpected_type<G> const &error ENABLE_IF_REQUIRES_A(
                    !std::is_convertible<G const &, E>::value /*=> explicit */
            )
            )
                    : has_value_(false) {
                contained.construct_error(E{error.value()});
            }

            template<typename G = E>
            constexpr /*non-explicit*/ expected(melon::unexpected_type<G> const &error ENABLE_IF_REQUIRES_A(
                    std::is_convertible<G const &, E>::value /*=> non-explicit */
            )
            )
                    : has_value_(false) {
                contained.construct_error(error.value());
            }

            template<typename G = E>
            constexpr explicit expected(melon::unexpected_type<G> &&error ENABLE_IF_REQUIRES_A(
                    !std::is_convertible<G &&, E>::value /*=> explicit */
            )
            )
                    : has_value_(false) {
                contained.construct_error(E{std::move(error.value())});
            }

            template<typename G = E>
            constexpr /*non-explicit*/ expected(melon::unexpected_type<G> &&error ENABLE_IF_REQUIRES_A(
                    std::is_convertible<G &&, E>::value /*=> non-explicit */
            )
            )
                    : has_value_(false) {
                contained.construct_error(std::move(error.value()));
            }

            template<typename... Args ENABLE_IF_REQUIRES_T(
                    std::is_constructible<E, Args && ...>::value
            )
            >
            constexpr explicit expected(unexpect_t, Args &&... args)
                    : has_value_(false) {
                contained.emplace_error(std::forward<Args>(args)...);
            }

            template<typename U, typename... Args ENABLE_IF_REQUIRES_T(
                    std::is_constructible<E, std::initializer_list<U>, Args && ...>::value
            )
            >
            constexpr explicit expected(unexpect_t, std::initializer_list<U> il, Args &&... args)
                    : has_value_(false) {
                contained.emplace_error(il, std::forward<Args>(args)...);
            }

            // destructor

            ~expected() {
                if (!has_value()) {
                    contained.destruct_error();
                }
            }

            // x.x.4.3 assignment

            ENABLE_IF_REQUIRES_R(
                    expected &,
                    std::is_copy_constructible<E>::value
                    && std::is_copy_assignable<E>::value
            )
            operator=(expected const &other) {
                expected(other).swap(*this);
                return *this;
            }

            ENABLE_IF_REQUIRES_R(
                    expected &,
                    std::is_move_constructible<E>::value
                    && std::is_move_assignable<E>::value
            )
            operator=(expected &&other) noexcept
            (
            std::is_nothrow_move_assignable<E>::value &&
            std::is_nothrow_move_constructible<E>::value) {
                expected(std::move(other)).swap(*this);
                return *this;
            }

            void emplace() {
                expected().swap(*this);
            }

            // x.x.4.4 swap

            ENABLE_IF_REQUIRES_R(void,
                            std::is_swappable<E>::value
                            && std::is_move_constructible<E>::value
            )
            swap(expected &other) noexcept
            (
            std::is_nothrow_move_constructible<E>::value && std::is_nothrow_swappable<E &>::value
            ) {
                using std::swap;

                if (!bool(*this) && !bool(other)) { swap(contained.error(), other.contained.error()); }
                else if (bool(*this) && !bool(other)) {
                    contained.construct_error(std::move(other.error()));
                    swap(has_value_, other.has_value_);
                } else if (!bool(*this) && bool(other)) { other.swap(*this); }
            }

            // x.x.4.5 observers

            constexpr explicit operator bool() const noexcept {
                return has_value();
            }

            constexpr bool has_value() const noexcept {
                return has_value_;
            }

            void value() const {
                if (!has_value()) {
                    error_traits<error_type>::rethrow(contained.error());
                }
            }

            constexpr error_type const &error() const &{
                return assert(!has_value()), contained.error();
            }

            error_type &error() &{
                return assert(!has_value()), contained.error();
            }

            constexpr error_type const &&error() const &&{
                return assert(!has_value()), std::move(contained.error());
            }

            error_type &&error() &&{
                return assert(!has_value()), std::move(contained.error());
            }


            constexpr unexpected_type get_unexpected() const {
                return make_unexpected(contained.error());
            }

            template<typename Ex>
            bool has_exception() const {
                using ContainedEx = typename std::remove_reference<decltype(get_unexpected().value())>::type;
                return !has_value() && std::is_base_of<Ex, ContainedEx>::value;
            }

//  template constexpr 'see below' unwrap() const&;
//
//  template 'see below' unwrap() &&;

            // factories

//  template< typename Ex, typename F>
//  expected<void,E> catch_exception(F&& f);
//
//  template< typename F>
//  expected<decltype(func()), E> map(F&& func) ;
//
//  template< typename F>
//  'see below' bind(F&& func) ;
//
//  template< typename F>
//  expected<void,E> catch_error(F&& f);
//
//  template< typename F>
//  'see below' then(F&& func);

        private:
            bool has_value_;
            detail::storage_t<void, E> contained;
        };

        // x.x.4.6 expected<>: comparison operators

        template<typename T1, typename E1, typename T2, typename E2>
        constexpr bool operator==(expected<T1, E1> const &x, expected<T2, E2> const &y) {
            return bool(x) != bool(y) ? false : bool(x) == false ? x.error() == y.error() : *x == *y;
        }

        template<typename T1, typename E1, typename T2, typename E2>
        constexpr bool operator!=(expected<T1, E1> const &x, expected<T2, E2> const &y) {
            return !(x == y);
        }

        template<typename E1, typename E2>
        constexpr bool operator==(expected<void, E1> const &x, expected<void, E1> const &y) {
            return bool(x) != bool(y) ? false : bool(x) == false ? x.error() == y.error() : true;
        }


        // x.x.4.7 expected: comparison with T

        template<typename T1, typename E1, typename T2>
        constexpr bool operator==(expected<T1, E1> const &x, T2 const &v) {
            return bool(x) ? *x == v : false;
        }

        template<typename T1, typename E1, typename T2>
        constexpr bool operator==(T2 const &v, expected<T1, E1> const &x) {
            return bool(x) ? v == *x : false;
        }

        template<typename T1, typename E1, typename T2>
        constexpr bool operator!=(expected<T1, E1> const &x, T2 const &v) {
            return bool(x) ? *x != v : true;
        }

        template<typename T1, typename E1, typename T2>
        constexpr bool operator!=(T2 const &v, expected<T1, E1> const &x) {
            return bool(x) ? v != *x : true;
        }

        // x.x.4.8 expected: comparison with unexpected_type

        template<typename T1, typename E1, typename E2>
        constexpr bool operator==(expected<T1, E1> const &x, unexpected_type<E2> const &u) {
            return (!x) ? x.get_unexpected() == u : false;
        }

        template<typename T1, typename E1, typename E2>
        constexpr bool operator==(unexpected_type<E2> const &u, expected<T1, E1> const &x) {
            return (x == u);
        }

        template<typename T1, typename E1, typename E2>
        constexpr bool operator!=(expected<T1, E1> const &x, unexpected_type<E2> const &u) {
            return !(x == u);
        }

        template<typename T1, typename E1, typename E2>
        constexpr bool operator!=(unexpected_type<E2> const &u, expected<T1, E1> const &x) {
            return !(x == u);
        }

        /// x.x.x Specialized algorithms

        template<typename T, typename E ENABLE_IF_REQUIRES_T(
                (std::is_void<T>::value || std::is_move_constructible<T>::value)
                && std::is_move_constructible<E>::value
                && std::is_swappable<T>::value
                && std::is_swappable<E>::value)
        >
        void swap(expected<T, E> &x, expected<T, E> &y) noexcept(noexcept(x.swap(y))) {
            x.swap(y);
        }

    } // namespace expected_lite

    using namespace expected_lite;

    /*
    template <typename T>
    using expected = expected<T, std::exception_ptr>;
    using unexpected = unexpected_type<std::exception_ptr>;
     */
    using unexpected = unexpected_type<std::exception_ptr>;

} // namespace melon

namespace std {

    // expected: hash support

    template<typename T, typename E>
    struct hash<melon::expected<T, E> > {
        using result_type = std::size_t;
        using argument_type = melon::expected<T, E>;

        constexpr result_type operator()(argument_type const &arg) const {
            return arg ? std::hash<T>{}(*arg) : result_type{};
        }
    };

    // TBD - ?? remove? see spec.
    template<typename T, typename E>
    struct hash<melon::expected<T &, E> > {
        using result_type = std::size_t;
        using argument_type = melon::expected<T &, E>;

        constexpr result_type operator()(argument_type const &arg) const {
            return arg ? std::hash<T>{}(*arg) : result_type{};
        }
    };

    // TBD - implement
    // bool(e), hash<expected<void,E>>()(e) shall evaluate to the hashing true;
    // otherwise it evaluates to an unspecified value if E is exception_ptr or
    // a combination of hashing false and hash<E>()(e.error()).

    template<typename E>
    struct hash<melon::expected<void, E> > {
    };

} // namespace std


#undef nsel_REQUIRES
#undef ENABLE_IF_REQUIRES_0
#undef ENABLE_IF_REQUIRES_T


#endif // MELON_FUTURE_EXPECTED_H_