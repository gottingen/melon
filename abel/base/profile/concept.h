//
// Created by liyinbin on 2020/3/22.
//

#ifndef ABEL_BASE_CONCEPT_H_
#define ABEL_BASE_CONCEPT_H_

#include <abel/base/profile/have.h>

#ifndef ABEL_CONCEPT
    #if defined(__cpp_concepts) && __cpp_concepts == 201507
        #define ABEL_CONCEPT(x...) x
        #define ABEL_NO_CONCEPT(x...)
    #else
        #define ABEL_CONCEPT(x...)
        #define ABEL_NO_CONCEPT(x...) x
    #endif
#endif

#endif //ABEL_BASE_CONCEPT_H_
