// Copyright (c) 2021, gottingen group.
// All rights reserved.
// Created by liyinbin lijippy@163.com

#ifndef ABEL_MATH_BIT_H_
#define ABEL_MATH_BIT_H_

namespace abel {

template<typename T>
bool has_single_bit(const T number) {
    return ((number != 0) && !(number & (number - 1)));
}

template<typename T>
T
bit_width(T number) {
    if (number == 0) return 0;

    T result = 1;
    T shifted_number = number;
    while (shifted_number >>= 1) {
        ++result;
    }
    return result;
}

}  // namespace abel

#endif  // ABEL_MATH_BIT_H_
