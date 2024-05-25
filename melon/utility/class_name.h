//
// Copyright (C) 2024 EA group inc.
// Author: Jeff.li lijippy@163.com
// All rights reserved.
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as published
// by the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.
//
//


// Date: Mon. Nov 7 14:47:36 CST 2011

// Get name of a class. For example, class_name<T>() returns the name of T
// (with namespace prefixes). This is useful in template classes.

#ifndef MUTIL_CLASS_NAME_H
#define MUTIL_CLASS_NAME_H

#include <typeinfo>
#include <string>                                // std::string

namespace mutil {

std::string demangle(const char* name);

namespace {
template <typename T> struct ClassNameHelper { static std::string name; };
template <typename T> std::string ClassNameHelper<T>::name = demangle(typeid(T).name());
}

// Get name of class |T|, in std::string.
template <typename T> const std::string& class_name_str() {
    // We don't use static-variable-inside-function because before C++11
    // local static variable is not guaranteed to be thread-safe.
    return ClassNameHelper<T>::name;
}

// Get name of class |T|, in const char*.
// Address of returned name never changes.
template <typename T> const char* class_name() {
    return class_name_str<T>().c_str();
}

// Get typename of |obj|, in std::string
template <typename T> std::string class_name_str(T const& obj) {
    return demangle(typeid(obj).name());
}

}  // namespace mutil

#endif  // MUTIL_CLASS_NAME_H
