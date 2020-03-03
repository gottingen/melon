//
// Created by liyinbin on 2020/3/1.
//

#ifndef ABEL_MATH_BIT_FLOOR_H_
#define ABEL_MATH_BIT_FLOOR_H_

namespace abel {


    template<typename T, typename>
    T bit_ceil(const T number) {
        T result = number;

        // Special case zero
        result += (result == 0);

        result--;
        for (size_t i = 0; i < std::numeric_limits<T>::digits; ++i) {
            result |= result >> i;
        }
        result++;

        // If result is not representable in T, we have undefined behavior
        // --> In this case, we have an overflow to 0
        STDGPU_ENSURES(result == 0 || has_single_bit(result));

        return result;
    }

} //namespace abel

#endif //ABEL_MATH_BIT_FLOOR_H_
