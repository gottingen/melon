//
// Created by liyinbin on 2020/2/28.
//

#ifndef ABEL_MATH_IS_INF_H_
#define ABEL_MATH_IS_INF_H_

#include <abel/math/option.h>

namespace abel {

    namespace math_internal {

        template<typename T>
        ABEL_CONSTEXPR bool is_neginf(const T x) ABEL_NOEXCEPT {
            return x == -std::numeric_limits<T>::infinity();
        }

        template<typename T1, typename T2>
        ABEL_CONSTEXPR bool any_neginf(const T1 x, const T2 y) ABEL_NOEXCEPT {
            return (is_neginf(x) || is_neginf(y));
        }

        template<typename T1, typename T2>
        ABEL_CONSTEXPR bool all_neginf(const T1 x, const T2 y) ABEL_NOEXCEPT {
            return (is_neginf(x) && is_neginf(y));
        }

        template<typename T1, typename T2, typename T3>
        ABEL_CONSTEXPR bool any_neginf(const T1 x, const T2 y, const T3 z) ABEL_NOEXCEPT {
            return (is_neginf(x) || is_neginf(y) || is_neginf(z));
        }

        template<typename T1, typename T2, typename T3>
        ABEL_CONSTEXPR bool all_neginf(const T1 x, const T2 y, const T3 z) ABEL_NOEXCEPT {
            return (is_neginf(x) && is_neginf(y) && is_neginf(z));
        }

//

        template<typename T>
        ABEL_CONSTEXPR bool is_posinf(const T x) ABEL_NOEXCEPT {
            return x == std::numeric_limits<T>::infinity();
        }

        template<typename T1, typename T2>
        ABEL_CONSTEXPR bool any_posinf(const T1 x, const T2 y) ABEL_NOEXCEPT {
            return (is_posinf(x) || is_posinf(y));
        }

        template<typename T1, typename T2>
        ABEL_CONSTEXPR bool all_posinf(const T1 x, const T2 y) ABEL_NOEXCEPT {
            return (is_posinf(x) && is_posinf(y));
        }

        template<typename T1, typename T2, typename T3>
        ABEL_CONSTEXPR bool any_posinf(const T1 x, const T2 y, const T3 z) ABEL_NOEXCEPT {
            return (is_posinf(x) || is_posinf(y) || is_posinf(z));
        }

        template<typename T1, typename T2, typename T3>
        ABEL_CONSTEXPR bool all_posinf(const T1 x, const T2 y, const T3 z) ABEL_NOEXCEPT {
            return (is_posinf(x) && is_posinf(y) && is_posinf(z));
        }


        template<typename T>
        ABEL_CONSTEXPR bool is_inf(const T x) ABEL_NOEXCEPT {
            return (is_neginf(x) || is_posinf(x));
        }

        template<typename T1, typename T2>
        ABEL_CONSTEXPR bool any_inf(const T1 x, const T2 y) ABEL_NOEXCEPT {
            return (is_inf(x) || is_inf(y));
        }

        template<typename T1, typename T2>
        ABEL_CONSTEXPR bool all_inf(const T1 x, const T2 y) ABEL_NOEXCEPT {
            return (is_inf(x) && is_inf(y));
        }

        template<typename T1, typename T2, typename T3>
        ABEL_CONSTEXPR bool any_inf(const T1 x, const T2 y, const T3 z) ABEL_NOEXCEPT {
            return (is_inf(x) || is_inf(y) || is_inf(z));
        }

        template<typename T1, typename T2, typename T3>
        ABEL_CONSTEXPR bool all_inf(const T1 x, const T2 y, const T3 z) ABEL_NOEXCEPT {
            return (is_inf(x) && is_inf(y) && is_inf(z));
        }

    }

}
#endif //ABEL_MATH_IS_INF_H_
