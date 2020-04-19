//
// Created by liyinbin on 2020/3/22.
//

#ifndef ABEL_ASL_INTERNAL_CONFIG_H_
#define ABEL_ASL_INTERNAL_CONFIG_H_

#include <abel/base/profile.h>
#include <type_traits>

#ifndef ASL_VARIABLE_TEMPLATES_ENABLED
    #if defined(ABEL_COMPILER_NO_VARIABLE_TEMPLATES)
        #define ASL_VARIABLE_TEMPLATES_ENABLED 0
    #else
        #define ASL_VARIABLE_TEMPLATES_ENABLED 1
    #endif
#endif

#ifndef ASL_TEMPLATE_ALIASES_ENABLED

    #ifdef ABEL_COMPILER_NO_TEMPLATE_ALIASES
        #define ASL_TEMPLATE_ALIASES_ENABLED 0
    #else
        #define ASL_TEMPLATE_ALIASES_ENABLED 1
    #endif

#endif //ASL_TEMPLATE_ALIASES_ENABLED


// In MSVC we can't probe std::hash or stdext::hash because it triggers a
// static_assert instead of failing substitution. Libc++ prior to 4.0
// also used a static_assert.
//
#if defined(_MSC_VER) || (defined(_LIBCPP_VERSION) && _LIBCPP_VERSION < 4000 && _LIBCPP_STD_VER > 11)
    #define ASL_STD_HASH_SFINAE_FRIENDLY 0
#else
    #define ASL_STD_HASH_SFINAE_FRIENDLY 1
#endif


#endif //ABEL_ASL_INTERNAL_CONFIG_H_
