// Copyright (c) 2021, gottingen group.
// All rights reserved.
// Created by liyinbin lijippy@163.com

#ifndef ABEL_META_TYPE_TRAITS_H_
#define ABEL_META_TYPE_TRAITS_H_

//    Trait(c++11)                          Description
// ------------------------------------------------------------------------------
//    is_void                               T is void or a cv-qualified (const/void-qualified) void.
//    is_integral                           T is an integral type.
//    is_floating_point                     T is a floating point type.
//    is_array                              T is an array type. The templated array container is not an array type.
//    is_enum                               T is an enumeration type.
//    is_union                              T is a union type.
//    is_class                              T is a class type but not a union type.
//    is_function         replace           T is a function type.
//    is_pointer                            T is a pointer type. Includes function pointers, but not pointers to (data or function) members.
//    is_rvalue_reference
//    is_lvalue_reference
//    is_member_object_pointer              T is a pointer to data member.
//    is_member_function_pointer            T is a pointer to member function.
//
//    is_fundamental                        T is a fundamental type (void, integral, or floating point).
//    is_arithmetic                         T is an arithmetic type (integral or floating point).
//    is_scalar                             T is a scalar type (arithmetic, enum, pointer, member_pointer)
//    is_object                             T is an object type.
//    is_compound                           T is a compound type (anything but fundamental).
//    is_reference                          T is a reference type. Includes references to functions.
//    is_member_pointer                     T is a pointer to a member or member function.
//
//    is_const                              T is const-qualified.
//    is_volatile                           T is volatile-qualified.
//    is_trivial
//    is_trivially_copyable    replace
//    is_standard_layout
//    is_pod                                T is a POD type.
//    is_literal_type
//    is_empty                              T is an empty class.
//    is_polymorphic                        T is a polymorphic class.
//    is_abstract                           T is an abstract class.
//    is_signed                             T is a signed integral type.
//    is_unsigned                           T is an unsigned integral type.
//
//    is_constructible
//    is_trivially_constructible
//    is_nothrow_constructible
//    is_default_constructible
//    is_trivially_default_constructible    replace
//    is_nothrow_default_constructible
//    is_copy_constructible
//    is_trivially_copy_constructible      replace
//    is_nothrow_copy_constructible
//    is_move_constructible
//    is_trivially_move_constructible     replace
//    is_nothrow_move_constructible
//    is_assignable
//    is_trivially_assignable
//    is_nothrow_assignable
//    is_copy_assignable             replace
//    is_trivially_copy_assignable   replace
//    is_nothrow_copy_assignable
//    is_move_assignable              replace
//    is_trivially_move_assignable    replace
//    is_nothrow_move_assignable
//    is_destructible
//    is_trivially_destructible       replace
//    is_nothrow_destructible
//    has_virtual_destructor                T has a virtual destructor.
//
//    alignment_of                          An integer value representing the number of bytes of the alignment of objects of type T; an object of type T may be allocated at an address that is a multiple of its alignment.
//    rank                                  An integer value representing the rank of objects of type T. The term 'rank' here is used to describe the number of dimensions of an array type.
//    extent                                An integer value representing the extent (dimension) of the I'th bound of objects of type T. If the type T is not an array type, has rank of less than I, or if I == 0 and T is of type 'array of unknown bound of U,' then value shall evaluate to zero; otherwise value shall evaluate to the number of elements in the I'th array bound of T. The term 'extent' here is used to describe the number of elements in an array type.
//
//    is_same                               T and U name the same type.
//    is_base_of                            Base is a base class of Derived or Base and Derived name the same type.
//    is_convertible                        An imaginary lvalue of type From is implicitly convertible to type To. Special conversions involving string-literals and null-pointer constants are not considered. No function-parameter adjustments are made to type To when determining whether From is convertible to To; this implies that if type To is a function type or an array type, then the condition is false.
//
//    remove_cv
//    remove_const                          The member typedef type shall be the same as T except that any top level const-qualifier has been removed. remove_const<const volatile int>::type evaluates to volatile int, whereas remove_const<const int*> is const int*.
//    remove_volatile
//    add_cv
//    add_const
//    add_volatile
//
//    remove_reference
//    add_lvalue_reference
//    add_rvalue_reference
//
//    remove_pointer
//    add_pointer
//
//    make_signed
//    make_unsigned
//
//    remove_extent
//    remove_all_extents
//
//    aligned_storage
//    aligned_union
//    decay
//    enable_if
//    conditional
//    common_type
//    underlying_type
//    result_of
//
//    integral_constant
//    true_type
//    false_type
//
//    Trait(c++14)                          Description
// ------------------------------------------------------------------------------
//    is_null_pointer
//
//    is_final
//
//    Trait(c++17)                          Description
// ------------------------------------------------------------------------------
//
//    has_unique_object_representations
//    is_aggregate
//
//    is_swappable_with
//    is_swappable                         compatible
//    is_nothrow_swappable_with
//    is_nothrow_swappable                 compatible
//
//    is_invocable
//    is_invocable_r
//    is_nothrow_invocable
//    is_nothrow_invocable_r
//
//    invoke_result
//    void_t                               compatible
//
//    conjunction                          compatible
//    disjunction                          compatible
//    negation                             compatible
//
//    bool_constant                        compatible
//
//    Trait(c++20)                          Description
// ------------------------------------------------------------------------------
//
//    is_bounded_array                      compatible
//    is_unbounded_array                    compatible
//
//    is_nothrow_convertible
//    is_layout_compatible
//    is_pointer_interconvertible_base_of
//    is_pointer_interconvertible_with_class
//    is_corresponding_member
//
//
//    remove_cvref                              compatible
//    common_reference
//    basic_common_reference
//    type_identity                             compatible
//
//    is_constant_evaluated
//
// abel extension type traits
//    is_widening_convertible
//    is_smart_ptr
//    is_hashable
//    assert_hash_enabled
//    is_detected
//    is_detected_convertible
//    implicit_cast


#include "abel/meta/internal/type_common.h"
#include "abel/meta/internal/type_pod.h"
#include "abel/meta/internal/type_fundamental.h"
#include "abel/meta/internal/type_compound.h"
#include "abel/meta/internal/type_transformation.h"
#include "abel/meta/internal/type_function.h"

#endif  // ABEL_META_TYPE_TRAITS_H_
