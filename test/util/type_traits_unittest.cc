// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <melon/base/type_traits.h>

#include <melon/base/basictypes.h>
#include <gtest/gtest.h>

namespace mutil {
namespace {

struct AStruct {};
class AClass {};
union AUnion {};
enum AnEnum { AN_ENUM_APPLE, AN_ENUM_BANANA, AN_ENUM_CARROT };
struct BStruct {
    int x;
};
class BClass {
#if defined(__clang__)
    int ALLOW_UNUSED _x;
#else
    int _x;
#endif
};

class Parent {};
class Child : public Parent {};

// is_empty<Type>
        static_assert(is_empty<AStruct>::value, "IsEmpty");
        static_assert(is_empty<AClass>::value, "IsEmpty");
        static_assert(!is_empty<BStruct>::value, "IsEmpty");
        static_assert(!is_empty<BClass>::value, "IsEmpty");

        // is_pointer<Type>
        static_assert(!is_pointer<int>::value, "IsPointer");
        static_assert(!is_pointer<int&>::value, "IsPointer");
        static_assert(is_pointer<int*>::value, "IsPointer");
        static_assert(is_pointer<const int*>::value, "IsPointer");

// is_array<Type>
        static_assert(!is_array<int>::value, "IsArray");
        static_assert(!is_array<int*>::value, "IsArray");
        static_assert(!is_array<int(*)[3]>::value, "IsArray");
        static_assert(is_array<int[]>::value, "IsArray");
        static_assert(is_array<const int[]>::value, "IsArray");
        static_assert(is_array<int[3]>::value, "IsArray");

// is_non_const_reference<Type>
        static_assert(!is_non_const_reference<int>::value, "IsNonConstReference");
        static_assert(!is_non_const_reference<const int&>::value, "IsNonConstReference");
        static_assert(is_non_const_reference<int&>::value, "IsNonConstReference");

// is_convertible<From, To>

// Extra parens needed to make preprocessor macro parsing happy. Otherwise,
// it sees the equivalent of:
//
//     (is_convertible < Child), (Parent > ::value)
//
// Silly C++.
        static_assert( (is_convertible<Child, Parent>::value), "IsConvertible");
        static_assert(!(is_convertible<Parent, Child>::value), "IsConvertible");
        static_assert(!(is_convertible<Parent, AStruct>::value), "IsConvertible");
        static_assert( (is_convertible<int, double>::value), "IsConvertible");
        static_assert( (is_convertible<int*, void*>::value), "IsConvertible");
        static_assert(!(is_convertible<void*, int*>::value), "IsConvertible");

// Array types are an easy corner case.  Make sure to test that
// it does indeed compile.
        static_assert(!(is_convertible<int[10], double>::value), "IsConvertible");
        static_assert(!(is_convertible<double, int[10]>::value), "IsConvertible");
        static_assert( (is_convertible<int[10], int*>::value), "IsConvertible");

// is_same<Type1, Type2>
        static_assert(!(is_same<Child, Parent>::value), "IsSame");
        static_assert(!(is_same<Parent, Child>::value), "IsSame");
        static_assert( (is_same<Parent, Parent>::value), "IsSame");
        static_assert( (is_same<int*, int*>::value), "IsSame");
        static_assert( (is_same<int, int>::value), "IsSame");
        static_assert( (is_same<void, void>::value), "IsSame");
        static_assert(!(is_same<int, double>::value), "IsSame");


// is_class<Type>
static_assert(is_class<AStruct>::value, "IsClass");
static_assert(is_class<AClass>::value, "IsClass");
static_assert(is_class<AUnion>::value, "IsClass");
static_assert(!is_class<AnEnum>::value, "IsClass");
static_assert(!is_class<int>::value, "IsClass");
static_assert(!is_class<char*>::value, "IsClass");
static_assert(!is_class<int&>::value, "IsClass");
static_assert(!is_class<char[3]>::value, "IsClass");

// NOTE(gejun): Not work in gcc 3.4 yet.
#if !defined(__GNUC__) || __GNUC__ >= 4
static_assert((is_enum<AnEnum>::value), "IsEnum");
#endif
static_assert(!(is_enum<AClass>::value), "IsEnum");
static_assert(!(is_enum<AStruct>::value), "IsEnum");
static_assert(!(is_enum<AUnion>::value), "IsEnum");

static_assert(!is_member_function_pointer<int>::value,
               "IsMemberFunctionPointer");
static_assert(!is_member_function_pointer<int*>::value,
               "IsMemberFunctionPointer");
static_assert(!is_member_function_pointer<void*>::value,
               "IsMemberFunctionPointer");
static_assert(!is_member_function_pointer<AStruct>::value,
               "IsMemberFunctionPointer");
static_assert(!is_member_function_pointer<AStruct*>::value,
               "IsMemberFunctionPointer");
static_assert(!is_member_function_pointer<int(*)(int)>::value,
               "IsMemberFunctionPointer");
static_assert(!is_member_function_pointer<int(*)(int, int)>::value,
               "IsMemberFunctionPointer");

static_assert(is_member_function_pointer<void (AStruct::*)()>::value,
               "IsMemberFunctionPointer");
static_assert(is_member_function_pointer<void (AStruct::*)(int)>::value,
               "IsMemberFunctionPointer");
static_assert(is_member_function_pointer<int (AStruct::*)(int)>::value,
               "IsMemberFunctionPointer");
static_assert(is_member_function_pointer<int (AStruct::*)(int) const>::value,
               "IsMemberFunctionPointer");
static_assert(is_member_function_pointer<int (AStruct::*)(int, int)>::value,
               "IsMemberFunctionPointer");
static_assert(is_member_function_pointer<
                 int (AStruct::*)(int, int) const>::value,
               "IsMemberFunctionPointer");
static_assert(is_member_function_pointer<
                 int (AStruct::*)(int, int, int)>::value,
               "IsMemberFunctionPointer");
static_assert(is_member_function_pointer<
                 int (AStruct::*)(int, int, int) const>::value,
               "IsMemberFunctionPointer");
static_assert(is_member_function_pointer<
                 int (AStruct::*)(int, int, int, int)>::value,
               "IsMemberFunctionPointer");
static_assert(is_member_function_pointer<
                 int (AStruct::*)(int, int, int, int) const>::value,
               "IsMemberFunctionPointer");

// False because we don't have a specialization for 5 params yet.
static_assert(!is_member_function_pointer<
                 int (AStruct::*)(int, int, int, int, int)>::value,
               "IsMemberFunctionPointer");
static_assert(!is_member_function_pointer<
                 int (AStruct::*)(int, int, int, int, int) const>::value,
               "IsMemberFunctionPointer");

// add_const
static_assert((is_same<add_const<int>::type, const int>::value), "AddConst");
static_assert((is_same<add_const<long>::type, const long>::value), "AddConst");
static_assert((is_same<add_const<std::string>::type, const std::string>::value),
               "AddConst");
static_assert((is_same<add_const<const int>::type, const int>::value),
               "AddConst");
static_assert((is_same<add_const<const long>::type, const long>::value),
               "AddConst");
static_assert((is_same<add_const<const std::string>::type,
                const std::string>::value), "AddConst");

// add_volatile
static_assert((is_same<add_volatile<int>::type, volatile int>::value),
               "AddVolatile");
static_assert((is_same<add_volatile<long>::type, volatile long>::value),
               "AddVolatile");
static_assert((is_same<add_volatile<std::string>::type,
                volatile std::string>::value), "AddVolatile");
static_assert((is_same<add_volatile<volatile int>::type, volatile int>::value),
               "AddVolatile");
static_assert((is_same<add_volatile<volatile long>::type, volatile long>::value),
               "AddVolatile");
static_assert((is_same<add_volatile<volatile std::string>::type,
                volatile std::string>::value), "AddVolatile");

// add_reference
static_assert((is_same<add_reference<int>::type, int&>::value), "AddReference");
static_assert((is_same<add_reference<long>::type, long&>::value), "AddReference");
static_assert((is_same<add_reference<std::string>::type, std::string&>::value),
               "AddReference");
static_assert((is_same<add_reference<int&>::type, int&>::value),
               "AddReference");
static_assert((is_same<add_reference<long&>::type, long&>::value),
               "AddReference");
static_assert((is_same<add_reference<std::string&>::type,
                std::string&>::value), "AddReference");
static_assert((is_same<add_reference<const int&>::type, const int&>::value),
               "AddReference");
static_assert((is_same<add_reference<const long&>::type, const long&>::value),
               "AddReference");
static_assert((is_same<add_reference<const std::string&>::type,
                const std::string&>::value), "AddReference");

// add_cr_non_integral
static_assert((is_same<add_cr_non_integral<int>::type, int>::value),
               "AddCrNonIntegral");
static_assert((is_same<add_cr_non_integral<long>::type, long>::value),
               "AddCrNonIntegral");
static_assert((is_same<add_cr_non_integral<std::string>::type,
                const std::string&>::value), "AddCrNonIntegral");
static_assert((is_same<add_cr_non_integral<const int>::type, const int&>::value),
               "AddCrNonIntegral");
static_assert((is_same<add_cr_non_integral<const long>::type, const long&>::value),
               "AddCrNonIntegral");
static_assert((is_same<add_cr_non_integral<const std::string>::type,
                const std::string&>::value), "AddCrNonIntegral");
static_assert((is_same<add_cr_non_integral<const int&>::type, const int&>::value),
               "AddCrNonIntegral");
static_assert((is_same<add_cr_non_integral<const long&>::type, const long&>::value),
               "AddCrNonIntegral");
static_assert((is_same<add_cr_non_integral<const std::string&>::type,
                const std::string&>::value), "AddCrNonIntegral");

// remove_const
static_assert((is_same<remove_const<const int>::type, int>::value),
               "RemoveConst");
static_assert((is_same<remove_const<const long>::type, long>::value),
               "RemoveConst");
static_assert((is_same<remove_const<const std::string>::type,
                std::string>::value), "RemoveConst");
static_assert((is_same<remove_const<int>::type, int>::value), "RemoveConst");
static_assert((is_same<remove_const<long>::type, long>::value), "RemoveConst");
static_assert((is_same<remove_const<std::string>::type, std::string>::value),
               "RemoveConst");

// remove_reference
static_assert((is_same<remove_reference<int&>::type, int>::value),
               "RemoveReference");
static_assert((is_same<remove_reference<long&>::type, long>::value),
               "RemoveReference");
static_assert((is_same<remove_reference<std::string&>::type,
                std::string>::value), "RemoveReference");
static_assert((is_same<remove_reference<const int&>::type, const int>::value),
               "RemoveReference");
static_assert((is_same<remove_reference<const long&>::type, const long>::value),
               "RemoveReference");
static_assert((is_same<remove_reference<const std::string&>::type,
                const std::string>::value), "RemoveReference");
static_assert((is_same<remove_reference<int>::type, int>::value), "RemoveReference");
static_assert((is_same<remove_reference<long>::type, long>::value), "RemoveReference");
static_assert((is_same<remove_reference<std::string>::type, std::string>::value),
               "RemoveReference");



}  // namespace
}  // namespace mutil
