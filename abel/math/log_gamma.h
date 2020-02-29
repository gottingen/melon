//
// Created by liyinbin on 2020/2/28.
//

#ifndef ABEL_LOG_GAMMA_H
#define ABEL_LOG_GAMMA_H

#include <abel/math/option.h>
#include <abel/math/is_nan.h>

namespace abel {

    namespace math_internal {

        // P. Godfrey's coefficients:
        //
        //  0.99999999999999709182
        //  57.156235665862923517
        // -59.597960355475491248
        //  14.136097974741747174
        //  -0.49191381609762019978
        //    .33994649984811888699e-4
        //    .46523628927048575665e-4
        //   -.98374475304879564677e-4
        //    .15808870322491248884e-3
        //   -.21026444172410488319e-3
        //    .21743961811521264320e-3
        //   -.16431810653676389022e-3
        //    .84418223983852743293e-4
        //   -.26190838401581408670e-4
        //    .36899182659531622704e-5

        ABEL_CONSTEXPR long double lgamma_coef_term(const long double x) ABEL_NOEXCEPT {
            return (0.99999999999999709182L + 57.156235665862923517L / (x + 1)
                    - 59.597960355475491248L / (x + 2) + 14.136097974741747174L / (x + 3)
                    - 0.49191381609762019978L / (x + 4) + .33994649984811888699e-4L / (x + 5)
                    + .46523628927048575665e-4L / (x + 6) - .98374475304879564677e-4L / (x + 7)
                    + .15808870322491248884e-3L / (x + 8) - .21026444172410488319e-3L / (x + 9)
                    + .21743961811521264320e-3L / (x + 10) - .16431810653676389022e-3L / (x + 11)
                    + .84418223983852743293e-4L / (x + 12) - .26190838401581408670e-4L / (x + 13)
                    + .36899182659531622704e-5L / (x + 14));
        }

        template<typename T>
        ABEL_CONSTEXPR T lgamma_term_2(const T x) ABEL_NOEXCEPT {
            return (T(ABEL_LOG_SQRT_2PI) + log(T(lgamma_coef_term(x))));
        }

        template<typename T>
        ABEL_CONSTEXPR T lgamma_term_1(const T x) ABEL_NOEXCEPT {
            return ((x + T(0.5)) * log(x + T(5.2421875L)) - (x + T(5.2421875L)));
        }

        template<typename T>
        ABEL_CONSTEXPR T lgamma_begin(const T x) ABEL_NOEXCEPT {
            return (lgamma_term_1(x) + lgamma_term_2(x));
        }

        template<typename T>
        ABEL_CONSTEXPR T lgamma_check(const T x) ABEL_NOEXCEPT {
            return (is_nan(x) ?
                    std::numeric_limits<T>::quiet_NaN() :
                    std::numeric_limits<T>::epsilon() > abs(x - T(1)) ?
                    T(0) :
                    std::numeric_limits<T>::epsilon() > x ?
                    std::numeric_limits<T>::infinity() :
                    lgamma_begin(x - T(1)));
        }

    }


    template<typename T>
    ABEL_CONSTEXPR return_t<T> lgamma(const T x) ABEL_NOEXCEPT {
        return math_internal::lgamma_check(static_cast<return_t<T>>(x));
    }
}
#endif //ABEL_LOG_GAMMA_H
