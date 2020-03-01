

#ifndef ABEL_MATH_OPTION_H_
#define ABEL_MATH_OPTION_H_

#include <cstddef>      // size_t
#include <limits>
#include <type_traits>
#include <abel/meta/type_traits.h>
#include <abel/base/profile.h>

namespace abel {

    template<typename T>
    using return_t = typename std::conditional<std::is_integral<T>::value, double, T>::type;

    template<typename ...T>
    using common_t = typename std::common_type<T...>::type;

    template<typename ...T>
    using common_return_t = return_t<common_t<T...>>;

}


#ifndef ABEL_LOG_2
#define ABEL_LOG_2 0.6931471805599453094172321214581765680755L
#endif

#ifndef ABEL_LOG_10
#define ABEL_LOG_10 2.3025850929940456840179914546843642076011L
#endif

#ifndef ABEL_PI
#define ABEL_PI 3.1415926535897932384626433832795028841972L
#endif

#ifndef ABEL_LOG_PI
#define ABEL_LOG_PI 1.1447298858494001741434273513530587116473L
#endif

#ifndef ABEL_LOG_2PI
#define ABEL_LOG_2PI 1.8378770664093454835606594728112352797228L
#endif

#ifndef ABEL_LOG_SQRT_2PI
#define ABEL_LOG_SQRT_2PI 0.9189385332046727417803297364056176398614L
#endif

#ifndef ABEL_SQRT_2
#define ABEL_SQRT_2 1.4142135623730950488016887242096980785697L
#endif

#ifndef ABEL_HALF_PI
#define ABEL_HALF_PI 1.5707963267948966192313216916397514420986L
#endif

#ifndef ABEL_SQRT_PI
#define ABEL_SQRT_PI 1.7724538509055160272981674833411451827975L
#endif

#ifndef ABEL_SQRT_HALF_PI
#define ABEL_SQRT_HALF_PI 1.2533141373155002512078826424055226265035L
#endif

#ifndef ABEL_E
#define ABEL_E 2.7182818284590452353602874713526624977572L
#endif


#ifndef ABEL_ERF_MAX_ITER
#define ABEL_ERF_MAX_ITER 60
#endif

#ifndef ABEL_ERF_INV_MAX_ITER
#define ABEL_ERF_INV_MAX_ITER 55
#endif

#ifndef ABEL_EXP_MAX_ITER_SMALL
#define ABEL_EXP_MAX_ITER_SMALL 25
#endif


#ifndef ABEL_LOG_MAX_ITER_SMALL
#define ABEL_LOG_MAX_ITER_SMALL 25
#endif

#ifndef ABEL_LOG_MAX_ITER_BIG
#define ABEL_LOG_MAX_ITER_BIG 255
#endif

#ifndef ABEL_INCML_BETA_TOL
#define ABEL_INCML_BETA_TOL 1E-15
#endif

#ifndef ABEL_INCML_BETA_MAX_ITER
#define ABEL_INCML_BETA_MAX_ITER 205
#endif

#ifndef ABEL_INCML_BETA_INV_MAX_ITER
#define ABEL_INCML_BETA_INV_MAX_ITER 35
#endif

#ifndef ABEL_INCML_GAMMA_MAX_ITER
#define ABEL_INCML_GAMMA_MAX_ITER 55
#endif

#ifndef ABEL_INCML_GAMMA_INV_MAX_ITER
#define ABEL_INCML_GAMMA_INV_MAX_ITER 35
#endif

#ifndef ABEL_SQRT_MAX_ITER
#define ABEL_SQRT_MAX_ITER 100
#endif

#ifndef ABEL_TAN_MAX_ITER
#define ABEL_TAN_MAX_ITER 35
#endif

#ifndef ABEL_TANH_MAX_ITER
#define ABEL_TANH_MAX_ITER 35
#endif


#ifdef _MSC_VER
#ifndef ABEL_SIGNBIT
        #define ABEL_SIGNBIT(x) _signbit(x)
    #endif
    #ifndef ABEL_COPYSIGN
        #define ABEL_COPYSIGN(x,y) _copysign(x,y)
    #endif
#else
#ifndef ABEL_SIGNBIT
#define ABEL_SIGNBIT(x) __builtin_signbit(x)
#endif
#ifndef ABEL_COPYSIGN
#define ABEL_COPYSIGN(x,y) __builtin_copysign(x,y)
#endif
#endif


#endif //ABEL_MATH_OPTION_H_
