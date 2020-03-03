//
// Created by liyinbin on 2020/2/28.
//

#ifndef ABEL_MATH_FIND_FRACTION_H_
#define ABEL_MATH_FIND_FRACTION_H_

#include <abel/math/option.h>

namespace abel {

    namespace math_internal {

        template<typename T>
        ABEL_CONSTEXPR T find_fraction(const T x) ABEL_NOEXCEPT {
            return (abs(x - math_internal::floor_check(x)) >= T(0.5) ? x - math_internal::floor_check(x) - sgn(x) :
                    x - math_internal::floor_check(x));
        }

    } //namespace math_internal

}
#endif //ABEL_MATH_FIND_FRACTION_H_
