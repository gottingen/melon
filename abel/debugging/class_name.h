// Copyright (c) 2021, gottingen group.
// All rights reserved.
// Created by liyinbin lijippy@163.com

#ifndef ABEL_DEBUGGING_CLASS_NAME_H_
#define ABEL_DEBUGGING_CLASS_NAME_H_

#include <typeinfo>
#include <string>                                // std::string

namespace abel {

    std::string demangle(const char *name);

    namespace debugging_internal {
        template<typename T>
        struct class_name_helper {
            static std::string name;
        };
        template<typename T> std::string class_name_helper<T>::name = demangle(typeid(T).name());
    }

    // Get name of class |T|, in std::string.
    template<typename T>
    const std::string &get_type_name() {
        // We don't use static-variable-inside-function because before C++11
        // local static variable is not guaranteed to be thread-safe.
        return debugging_internal::class_name_helper<T>::name;
    }

    // Get name of class |T|, in const char*.
    // Address of returned name never changes.
    template<typename T>
    const char *get_type_name_cstr() {
        return get_type_name<T>().c_str();
    }

    // Get typename of |obj|, in std::string
    template<typename T>
    std::string get_type_name(T const &obj) {
        return demangle(typeid(obj).name());
    }

}  // namespace abel

#endif  // ABEL_DEBUGGING_CLASS_NAME_H_
