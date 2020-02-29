//
// Created by liyinbin on 2020/2/28.
//

#ifndef ABEL_FIND_WHOLE_H
#define ABEL_FIND_WHOLE_H

#include <abel/math/option.h>
#include <abel/math/floor.h>
#include <abel/math/sgn.h>

namespace abel {

    namespace math_internal {

        template<typename T>
        ABEL_CONSTEXPR long long find_whole(const T x) ABEL_NOEXCEPT {
            return (abs(x - math_internal::floor_check(x)) >= T(0.5) ?
                    static_cast<long long>(math_internal::floor_check(x) + sgn(x)) :
                    static_cast<long long>(math_internal::floor_check(x)));
        }

    } //namespace abel

}
#endif //ABEL_FIND_WHOLE_H
