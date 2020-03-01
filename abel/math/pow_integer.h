

#ifndef ABEL_MATH_POW_INTEGER_H_
#define ABEL_MATH_POW_INTEGER_H_

#include <abel/math/option.h>
#include <abel/math/is_odd.h>

namespace abel {

    namespace math_internal {


        template<typename T1, typename T2>
        ABEL_CONSTEXPR T1 pow_integral_compute(const T1 base, const T2 exp_term) ABEL_NOEXCEPT;

        // integral-valued powers using method described in
        // https://en.wikipedia.org/wiki/Exponentiation_by_squaring

        template<typename T1, typename T2>
        ABEL_CONSTEXPR T1
        pow_integral_compute_recur(const T1 base, const T1 val, const T2 exp_term) ABEL_NOEXCEPT {
            return (exp_term > T2(1) ? (is_odd(exp_term) ?
                pow_integral_compute_recur(base * base, val * base, exp_term / 2) :
                    pow_integral_compute_recur(base * base, val, exp_term / 2)) :
                    (exp_term == T2(1) ? val * base : val));
        }

        template<typename T1, typename T2, typename std::enable_if<std::is_signed<T2>::value>::type * = nullptr>
        ABEL_CONSTEXPR T1 pow_integral_sgn_check(const T1 base, const T2 exp_term) ABEL_NOEXCEPT {
            return (exp_term < T2(0) ? T1(1) / pow_integral_compute(base, -exp_term) :
                    pow_integral_compute_recur(base, T1(1), exp_term)
                    );
        }

        template<typename T1, typename T2, typename std::enable_if<!std::is_signed<T2>::value>::type * = nullptr>
        ABEL_CONSTEXPR T1
        pow_integral_sgn_check(const T1 base, const T2 exp_term) ABEL_NOEXCEPT {
            return (pow_integral_compute_recur(base, T1(1), exp_term));
        }

        template<typename T1, typename T2>
        ABEL_CONSTEXPR T1 pow_integral_compute(const T1 base, const T2 exp_term) ABEL_NOEXCEPT {
            return (exp_term == T2(3) ? base * base * base : exp_term == T2(2) ?
                base * base : exp_term == T2(1) ?
                base : exp_term == T2(0) ? T1(1) : exp_term == std::numeric_limits<T2>::min() ?
                T1(0) : exp_term == std::numeric_limits<T2>::max() ?
                std::numeric_limits<T1>::infinity() : pow_integral_sgn_check(base, exp_term)
                );
        }

        template<typename T1, typename T2, typename std::enable_if<std::is_integral<T2>::value>::type * = nullptr>
        ABEL_CONSTEXPR T1 pow_integral_type_check(const T1 base, const T2 exp_term) ABEL_NOEXCEPT {
            return pow_integral_compute(base, exp_term);
        }

        template<typename T1, typename T2, typename std::enable_if<!std::is_integral<T2>::value>::type * = nullptr>
        ABEL_CONSTEXPR T1 pow_integral_type_check(const T1 base, const T2 exp_term) ABEL_NOEXCEPT {
            // return std::numeric_limits<return_t<T1>>::quiet_NaN();
            return pow_integral_compute(base, static_cast<long long>(exp_term));
        }

        template<typename T1, typename T2>
        ABEL_CONSTEXPR T1 pow_integral(const T1 base, const T2 exp_term) ABEL_NOEXCEPT {
            return math_internal::pow_integral_type_check(base, exp_term);
        }

    } //namespace math_internal

} //namespace abel

#endif //ABEL_MATH_POW_INTEGER_H_
