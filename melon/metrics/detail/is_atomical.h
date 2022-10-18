
#ifndef MELON_VARIABLE_DETAIL_IS_ATOMICAL_H_
#define MELON_VARIABLE_DETAIL_IS_ATOMICAL_H_

#include "melon/base/type_traits.h"

namespace melon {
    namespace metrics_detail {
        template<class T>
        struct is_atomical
                : std::integral_constant<bool, (std::is_integral<T>::value ||
                                                std::is_floating_point<T>::value)> {
        };
        template<class T>
        struct is_atomical<const T> : is_atomical<T> {
        };
        template<class T>
        struct is_atomical<volatile T> : is_atomical<T> {
        };
        template<class T>
        struct is_atomical<const volatile T> : is_atomical<T> {
        };

    }  // namespace metrics_detail
}  // namespace melon

#endif
