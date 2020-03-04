//
// Created by liyinbin on 2020/2/28.
//

#ifndef ABEL_MATH_LOG_H_
#define ABEL_MATH_LOG_H_

#include <abel/math/option.h>
#include <abel/math/is_nan.h>
#include <abel/math/mantissa.h>
#include <abel/math/find_exponent.h>
#include <abel/math/abs.h>

namespace abel {

    namespace math_internal {

        // continued fraction seems to be a better approximation for small x
        // see http://functions.wolfram.com/ElementaryFunctions/Log/10/0005/

        template<typename T>
        ABEL_CONSTEXPR T log_cf_main(const T xx, const int depth) ABEL_NOEXCEPT {
            return (depth < ABEL_LOG_MAX_ITER_SMALL ?
                    T(2 * depth - 1) - T(depth * depth) * xx / log_cf_main(xx, depth + 1) : T(2 * depth - 1));
        }

        template<typename T>
        ABEL_CONSTEXPR T log_cf_begin(const T x) ABEL_NOEXCEPT {
            return (T(2) * x / log_cf_main(x * x, 1));
        }

        template<typename T>
        ABEL_CONSTEXPR T log_main(const T x) ABEL_NOEXCEPT {
            return (log_cf_begin((x - T(1)) / (x + T(1))));
        }

        ABEL_CONSTEXPR long double log_mantissa_integer(const int x) ABEL_NOEXCEPT {
            return (x == 2 ? 0.6931471805599453094172321214581765680755L :
                    x == 3 ? 1.0986122886681096913952452369225257046475L :
                    x == 4 ? 1.3862943611198906188344642429163531361510L :
                    x == 5 ? 1.6094379124341003746007593332261876395256L :
                    x == 6 ? 1.7917594692280550008124773583807022727230L :
                    x == 7 ? 1.9459101490553133051053527434431797296371L :
                    x == 8 ? 2.0794415416798359282516963643745297042265L :
                    x == 9 ? 2.1972245773362193827904904738450514092950L :
                    x == 10 ? 2.3025850929940456840179914546843642076011L :
                    0.0L);
        }

        template<typename T>
        ABEL_CONSTEXPR T log_mantissa(const T x) ABEL_NOEXCEPT {
            return (log_main(x / T(static_cast<int>(x))) + T(log_mantissa_integer(static_cast<int>(x))));
        }

        template<typename T>
        ABEL_CONSTEXPR T log_breakup(const T x) ABEL_NOEXCEPT {
            return (log_mantissa(mantissa(x)) + T(ABEL_LOG_10) * T(find_exponent(x, 0)));
        }

        template<typename T>
        ABEL_CONSTEXPR T log_check(const T x) ABEL_NOEXCEPT {
            return (is_nan(x) ?
                    std::numeric_limits<T>::quiet_NaN() : x < T(0) ?
                                                          std::numeric_limits<T>::quiet_NaN() :
                                                          std::numeric_limits<T>::epsilon() > x ?
                                                          -std::numeric_limits<T>::infinity() :
                                                          std::numeric_limits<T>::epsilon() > abs(x - T(1)) ?
                                                          T(0) : x == std::numeric_limits<T>::infinity() ?
                                                                 std::numeric_limits<T>::infinity() :
                                                                 (x < T(0.5) || x > T(1.5)) ?
                                                                 log_breakup(x) :
                                                                 log_main(x));
        }

    } //namespace math_internal


    template<typename T>
    ABEL_CONSTEXPR return_t<T> log(const T x) ABEL_NOEXCEPT {
        return math_internal::log_check(static_cast<return_t<T>>(x));
    }



// log2_floor()

//! calculate the log2 floor of an integer type
    template<typename IntegerType>
    ABEL_CONSTEXPR unsigned log2_floor(IntegerType n) {
        return (n <= 1) ? 0 : 1 + log2_floor(n / 2);
    }

/******************************************************************************/
// log2_ceil()

//! calculate the log2 floor of an integer type
    template<typename IntegerType>
    ABEL_CONSTEXPR unsigned log2_ceil(IntegerType n) {
        return (n <= 1) ? 0 : log2_floor(n - 1) + 1;
    }

    ABEL_FORCE_INLINE double stirling_log_factorial(double n) {
        assert(n >= 1);
        // Using Stirling's approximation.
        constexpr double kLog2PI = 1.83787706640934548356;
        const double logn = log(n);
        const double ninv = 1.0 / static_cast<double>(n);
        return n * logn - n + 0.5 * (kLog2PI + logn) + (1.0 / 12.0) * ninv -
               (1.0 / 360.0) * ninv * ninv * ninv;
    }

    template<typename T>
    ABEL_CONSTEXPR T log_if(const T x, const bool log_form) ABEL_NOEXCEPT {
        return log_form ? abel::log(x) : x;
    }

    template<typename T>
    ABEL_CONSTEXPR T log_zero_if(const bool log_form) ABEL_NOEXCEPT {
        return log_form ? - std::numeric_limits<T>::infinity() : T(0);
    }

    template<typename T>
    ABEL_CONSTEXPR T log_one_if(const bool log_form) ABEL_NOEXCEPT {
        return log_form ? T(0) : T(1);
    }

} //namespace abel

#endif //ABEL_MATH_LOG_H_
