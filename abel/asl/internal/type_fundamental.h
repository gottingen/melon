//
// Created by liyinbin on 2020/3/22.
//

#ifndef ABEL_ASL_INTERANL_TYPE_FUNDAMENTAL_H_
#define ABEL_ASL_INTERANL_TYPE_FUNDAMENTAL_H_

#include <abel/asl/internal/config.h>

namespace abel {

    template<typename T>
    struct is_smart_ptr : std::false_type {
    };

    template<typename T>
    struct is_smart_ptr<std::unique_ptr<T>> : std::true_type {
    };

} //namespace abel

#endif //ABEL_ASL_INTERANL_TYPE_FUNDAMENTAL_H_
